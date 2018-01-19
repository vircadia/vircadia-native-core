//
//  AssetScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-03-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetScriptingInterface.h"

#include <QMimeDatabase>
#include <QThread>
#include <QtScript/QScriptEngine>

#include <AssetRequest.h>
#include <AssetUpload.h>
#include <AssetUtils.h>
#include <MappingRequest.h>
#include <NodeList.h>

#include <RegisteredMetaTypes.h>

#include <shared/QtHelpers.h>
#include "Gzip.h"
#include "ScriptEngine.h"
#include "ScriptEngineLogging.h"

AssetScriptingInterface::AssetScriptingInterface(QObject* parent) : BaseAssetScriptingInterface(parent) {}

#define JS_VERIFY(cond, error) { if (!this->jsVerify(cond, error)) { return; } }

void AssetScriptingInterface::uploadData(QString data, QScriptValue callback) {
    auto handler = makeScopedHandlerObject(thisObject(), callback);
    QByteArray dataByteArray = data.toUtf8();
    auto upload = DependencyManager::get<AssetClient>()->createUpload(dataByteArray);

    Promise deferred = makePromise(__FUNCTION__);
    deferred->ready([this, handler](QString error, QVariantMap result) {
        auto url = result.value("url").toString();
        auto hash = result.value("hash").toString();
        jsCallback(handler, url, hash);
    });

    connect(upload, &AssetUpload::finished, upload, [this, deferred](AssetUpload* upload, const QString& hash) {
        // we are now on the "Resource Manager" thread (and "hash" being a *reference* makes it unsafe to use directly)
        Q_ASSERT(QThread::currentThread() == upload->thread());
        deferred->resolve(NoError, {
            { "url", "atp:" + hash },
            { "hash", hash },
        });
        upload->deleteLater();
    });
    upload->start();
}

void AssetScriptingInterface::setMapping(QString path, QString hash, QScriptValue callback) {
    auto handler = makeScopedHandlerObject(thisObject(), callback);
    auto setMappingRequest = assetClient()->createSetMappingRequest(path, hash);
    Promise deferred = makePromise(__FUNCTION__);
    deferred->ready([=](QString error, QVariantMap result) {
        jsCallback(handler, error, result);
    });

    connect(setMappingRequest, &SetMappingRequest::finished, setMappingRequest, [this, deferred](SetMappingRequest* request) {
        Q_ASSERT(QThread::currentThread() == request->thread());
        // we are now on the "Resource Manager" thread
        QString error = request->getErrorString();
        // forward a thread-safe values back to our thread
        deferred->handle(error, { { "error", request->getError() } });
        request->deleteLater();
    });
    setMappingRequest->start();
}

void AssetScriptingInterface::downloadData(QString urlString, QScriptValue callback) {
    // FIXME: historically this API method failed silently when given a non-atp prefixed
    //   urlString (or if the AssetRequest failed).
    // .. is that by design or could we update without breaking things to provide better feedback to scripts?

    if (!urlString.startsWith(ATP_SCHEME)) {
        // ... for now at least log a message so user can check logs
        qCDebug(scriptengine) << "AssetScriptingInterface::downloadData url must be of form atp:<hash-value>";
        return;
    }
    QString hash = AssetUtils::extractAssetHash(urlString);
    auto handler = makeScopedHandlerObject(thisObject(), callback);
    auto assetClient = DependencyManager::get<AssetClient>();
    auto assetRequest = assetClient->createRequest(hash);

    Promise deferred = makePromise(__FUNCTION__);
    deferred->ready([=](QString error, QVariantMap result) {
        // FIXME: to remain backwards-compatible the signature here is "callback(data, n/a)"
        jsCallback(handler, result.value("data").toString(), { { "errorMessage", error } });
    });

    connect(assetRequest, &AssetRequest::finished, assetRequest, [this, deferred](AssetRequest* request) {
        Q_ASSERT(QThread::currentThread() == request->thread());
        // we are now on the "Resource Manager" thread
        Q_ASSERT(request->getState() == AssetRequest::Finished);

        if (request->getError() == AssetRequest::Error::NoError) {
            QString data = QString::fromUtf8(request->getData());
            // forward a thread-safe values back to our thread
            deferred->resolve(NoError, { { "data", data } });
        } else {
            // FIXME: propagate error to scripts? (requires changing signature or inverting param order above..)
            //deferred->resolve(request->getErrorString(), { { "error", requet->getError() } });
            qDebug() << "AssetScriptingInterface::downloadData ERROR: " << request->getErrorString();
        }

        request->deleteLater();
    });

    assetRequest->start();
}

void AssetScriptingInterface::setBakingEnabled(QString path, bool enabled, QScriptValue callback) {
    auto handler = makeScopedHandlerObject(thisObject(), callback);
    auto setBakingEnabledRequest = DependencyManager::get<AssetClient>()->createSetBakingEnabledRequest({ path }, enabled);

    Promise deferred = makePromise(__FUNCTION__);
    deferred->ready([=](QString error, QVariantMap result) {
        jsCallback(handler, error, result);
    });

    connect(setBakingEnabledRequest, &SetBakingEnabledRequest::finished, setBakingEnabledRequest, [this, deferred](SetBakingEnabledRequest* request) {
        Q_ASSERT(QThread::currentThread() == request->thread());
        // we are now on the "Resource Manager" thread

        QString error = request->getErrorString();
        // forward thread-safe values back to our thread
        deferred->handle(error, {});
        request->deleteLater();
    });
    setBakingEnabledRequest->start();
}

#if (PR_BUILD || DEV_BUILD)
void AssetScriptingInterface::sendFakedHandshake() {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    nodeList->sendFakedHandshakeRequestToNode(assetServer);
}

#endif

void AssetScriptingInterface::getMapping(QString asset, QScriptValue callback) {
    auto path = AssetUtils::getATPUrl(asset).path();
    auto handler = makeScopedHandlerObject(thisObject(), callback);
    JS_VERIFY(AssetUtils::isValidFilePath(path), "invalid ATP file path: " + asset + "(path:"+path+")");
    JS_VERIFY(callback.isFunction(), "expected second parameter to be a callback function");
    qDebug() << ">>getMapping//getAssetInfo" << path;
    Promise promise = getAssetInfo(path);
    promise->ready([this, handler](QString error, QVariantMap result) {
        qDebug() << "//getMapping//getAssetInfo" << error << result.keys();
        jsCallback(handler, error, result.value("hash").toString());
    });
}

bool AssetScriptingInterface::jsVerify(bool condition, const QString& error) {
    if (condition) {
        return true;
    }
    if (context()) {
        context()->throwError(error);
    } else {
        qDebug() << "WARNING -- jsVerify failed outside of a valid JS context: " + error;
    }
    return false;
}

void AssetScriptingInterface::jsCallback(const QScriptValue& handler,
                                         const QScriptValue& error, const QScriptValue& result) {
    Q_ASSERT(thread() == QThread::currentThread());
    auto errorValue = !error.toBool() ? QScriptValue::NullValue : error;
    JS_VERIFY(handler.isObject() && handler.property("callback").isFunction(),
              QString("jsCallback -- .callback is not a function (%1)")
              .arg(handler.property("callback").toVariant().typeName()));
    ::callScopedHandlerObject(handler, errorValue, result);
}

void AssetScriptingInterface::jsCallback(const QScriptValue& handler,
                                         const QScriptValue& error, const QVariantMap& result) {
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(handler.engine());
    auto engine = handler.engine();
    jsCallback(handler, error, engine->toScriptValue(result));
}

void AssetScriptingInterface::deleteAsset(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    jsVerify(false, "TODO: deleteAsset API");
}

void AssetScriptingInterface::getAsset(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    JS_VERIFY(options.isObject() || options.isString(), "expected request options Object or URL as first parameter");

    auto decompress = options.property("decompress").toBool() || options.property("compressed").toBool();
    auto responseType = options.property("responseType").toString().toLower();
    auto url = options.property("url").toString();
    if (options.isString()) {
        url = options.toString();
    }
    if (responseType.isEmpty() || responseType == "string") {
        responseType = "text";
    }
    auto asset = AssetUtils::getATPUrl(url).path();
    auto handler = makeScopedHandlerObject(scope, callback);

    JS_VERIFY(handler.property("callback").isFunction(),
              QString("Invalid callback function (%1)").arg(handler.property("callback").toVariant().typeName()));
    JS_VERIFY(AssetUtils::isValidHash(asset) || AssetUtils::isValidFilePath(asset),
              QString("Invalid ATP url '%1'").arg(url));
    JS_VERIFY(RESPONSE_TYPES.contains(responseType),
              QString("Invalid responseType: '%1' (expected: %2)").arg(responseType).arg(RESPONSE_TYPES.join(" | ")));

    Promise resolved = makePromise("resolved");
    Promise loaded = makePromise("loaded");

    loaded->ready([=](QString error, QVariantMap result) {
        qDebug() << "//loaded" << error;
        jsCallback(handler, error, result);
    });

    resolved->ready([=](QString error, QVariantMap result) {
        qDebug() << "//resolved" << result.value("hash");
        QString hash = result.value("hash").toString();
        if (!error.isEmpty() || !AssetUtils::isValidHash(hash)) {
            loaded->reject(error.isEmpty() ? "internal hash error: " + hash : error, result);
        } else {
            Promise promise = loadAsset(hash, decompress, responseType);
            promise->mixin(result);
            promise->ready([this, loaded, hash](QString error, QVariantMap result) {
                qDebug() << "//getAssetInfo/loadAsset" << error << hash;
                loaded->resolve(NoError, result);
            });
        }
    });

    if (AssetUtils::isValidHash(asset)) {
        resolved->resolve(NoError, { { "hash", asset } });
    } else {
        Promise promise = getAssetInfo(asset);
        promise->ready([this, resolved](QString error, QVariantMap result) {
            qDebug() << "//getAssetInfo" << error << result.value("hash") << result.value("path");
            resolved->resolve(error, result);
        });
    }
}

void AssetScriptingInterface::resolveAsset(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    const QString& URL{ "url" };

    auto url = (options.isString() ? options : options.property(URL)).toString();
    auto asset = AssetUtils::getATPUrl(url).path();
    auto handler = makeScopedHandlerObject(scope, callback);

    JS_VERIFY(AssetUtils::isValidFilePath(asset) || AssetUtils::isValidHash(asset),
             "expected options to be an asset URL or request options containing .url property");
    JS_VERIFY(handler.property("callback").isFunction(), "invalid callback function");
    getAssetInfo(asset)->ready([=](QString error, QVariantMap result) {
        qDebug() << "//resolveAsset/getAssetInfo" << error << result.value("hash");
        jsCallback(handler, error, result);
    });
}

void AssetScriptingInterface::decompressData(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    auto data = options.property("data");
    QByteArray dataByteArray = qscriptvalue_cast<QByteArray>(data);
    auto handler = makeScopedHandlerObject(scope, callback);
    auto responseType = options.property("responseType").toString().toLower();
    if (responseType.isEmpty()) {
        responseType = "text";
    }
    Promise promise = decompressBytes(dataByteArray);
    promise->ready([=](QString error, QVariantMap result) {
        if (responseType == "arraybuffer") {
            jsCallback(handler, error, result);
        } else {
            Promise promise = convertBytes(result.value("data").toByteArray(), responseType);
            promise->mixin(result);
            promise->ready([=](QString error, QVariantMap result) {
                jsCallback(handler, error, result);
            });
        }
    });
}

namespace {
    const int32_t DEFAULT_GZIP_COMPRESSION_LEVEL = -1;
    const int32_t MAX_GZIP_COMPRESSION_LEVEL = 9;
}

void AssetScriptingInterface::compressData(QScriptValue options, QScriptValue scope, QScriptValue callback) {

    auto data = options.property("data");
    QByteArray dataByteArray = data.isString() ?
        data.toString().toUtf8() :
        qscriptvalue_cast<QByteArray>(data);
    auto handler = makeScopedHandlerObject(scope, callback);
    auto level = options.property("level").toInt32();
    if (level < DEFAULT_GZIP_COMPRESSION_LEVEL || level > MAX_GZIP_COMPRESSION_LEVEL) {
        level = DEFAULT_GZIP_COMPRESSION_LEVEL;
    }
    Promise promise = compressBytes(dataByteArray, level);
    promise->ready([=](QString error, QVariantMap result) {
        jsCallback(handler, error, result);
    });
}

void AssetScriptingInterface::putAsset(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    auto compress = options.property("compress").toBool() ||
        options.property("compressed").toBool();
    auto handler = makeScopedHandlerObject(scope, callback);
    auto data = options.property("data");
    auto rawPath = options.property("path").toString();
    auto path = AssetUtils::getATPUrl(rawPath).path();

    QByteArray dataByteArray = data.isString() ?
        data.toString().toUtf8() :
        qscriptvalue_cast<QByteArray>(data);

    JS_VERIFY(path.isEmpty() || AssetUtils::isValidFilePath(path),
              QString("expected valid ATP file path '%1' ('%2')").arg(rawPath).arg(path));
    JS_VERIFY(handler.property("callback").isFunction(),
              "invalid callback function");
    JS_VERIFY(dataByteArray.size() > 0,
              QString("expected non-zero .data (got %1 / #%2 bytes)")
              .arg(data.toVariant().typeName())
              .arg(dataByteArray.size()));

    // [compressed] => uploaded to server => [mapped to path]
    Promise prepared = makePromise("putAsset::prepared");
    Promise uploaded = makePromise("putAsset::uploaded");
    Promise finished = makePromise("putAsset::finished");

    if (compress) {
        qDebug() << "putAsset::compressBytes...";
        Promise promise = compressBytes(dataByteArray, DEFAULT_GZIP_COMPRESSION_LEVEL);
        promise->finally([=](QString error, QVariantMap result) {
            qDebug() << "//putAsset::compressedBytes" << error << result.keys();
            prepared->handle(error, result);
        });
    } else {
        prepared->resolve(NoError, {{ "data", dataByteArray }});
    }

    prepared->ready([=](QString error, QVariantMap result) {
        qDebug() << "//putAsset::prepared" << error << result.value("data").toByteArray().size() << result.keys();
        Promise promise = uploadBytes(result.value("data").toByteArray());
        promise->mixin(result);
        promise->ready([=](QString error, QVariantMap result) {
            qDebug() << "===========//putAsset::prepared/uploadBytes" << error << result.keys();
            uploaded->handle(error, result);
        });
    });

    uploaded->ready([=](QString error, QVariantMap result) {
        QString hash = result.value("hash").toString();
        qDebug() << "//putAsset::uploaded" << error << hash << result.keys();
        if (path.isEmpty()) {
            finished->handle(error, result);
        } else if (!AssetUtils::isValidHash(hash)) {
            finished->reject("path mapping requested, but did not receive valid hash", result);
        } else {
            qDebug() << "symlinkAsset" << hash << path << QThread::currentThread();
            Promise promise = symlinkAsset(hash, path);
            promise->mixin(result);
            promise->ready([=](QString error, QVariantMap result) {
                finished->handle(error, result);
                qDebug() << "//symlinkAsset" << hash << path << result.keys();
            });
        }
    });

    finished->ready([=](QString error, QVariantMap result) {
        qDebug() << "//putAsset::finished" << error << result.keys();
        jsCallback(handler, error, result);
    });
}
