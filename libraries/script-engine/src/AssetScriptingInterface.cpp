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
#include <BaseScriptEngine.h>
#include <MappingRequest.h>
#include <NodeList.h>

#include <RegisteredMetaTypes.h>
#include "ScriptEngine.h"
#include "ScriptEngineLogging.h"

#include <shared/QtHelpers.h>
#include <Gzip.h>

using Promise = MiniPromise::Promise;

AssetScriptingInterface::AssetScriptingInterface(QObject* parent) : BaseAssetScriptingInterface(parent) {
    qCDebug(scriptengine) << "AssetScriptingInterface::AssetScriptingInterface" << parent;
    MiniPromise::registerMetaTypes(parent);
}

#define JS_VERIFY(cond, error) { if (!this->jsVerify(cond, error)) { return; } }

void AssetScriptingInterface::uploadData(QString data, QScriptValue callback) {
    auto handler = jsBindCallback(thisObject(), callback);
    QByteArray dataByteArray = data.toUtf8();
    auto upload = DependencyManager::get<AssetClient>()->createUpload(dataByteArray);

    Promise deferred = makePromise(__FUNCTION__);
    deferred->ready([=](QString error, QVariantMap result) {
        auto url = result.value("url").toString();
        auto hash = result.value("hash").toString();
        jsCallback(handler, url, hash);
    });

    connect(upload, &AssetUpload::finished, upload, [deferred](AssetUpload* upload, const QString& hash) {
        // we are now on the "Resource Manager" thread (and "hash" being a *reference* makes it unsafe to use directly)
        Q_ASSERT(QThread::currentThread() == upload->thread());
        deferred->resolve({
            { "url", "atp:" + hash },
            { "hash", hash },
        });
        upload->deleteLater();
    });
    upload->start();
}

void AssetScriptingInterface::setMapping(QString path, QString hash, QScriptValue callback) {
    auto handler = jsBindCallback(thisObject(), callback);
    auto setMappingRequest = assetClient()->createSetMappingRequest(path, hash);
    Promise deferred = makePromise(__FUNCTION__);
    deferred->ready([=](QString error, QVariantMap result) {
        jsCallback(handler, error, result);
    });

    connect(setMappingRequest, &SetMappingRequest::finished, setMappingRequest, [deferred](SetMappingRequest* request) {
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
    auto handler = jsBindCallback(thisObject(), callback);
    auto assetClient = DependencyManager::get<AssetClient>();
    auto assetRequest = assetClient->createRequest(hash);

    Promise deferred = makePromise(__FUNCTION__);
    deferred->ready([=](QString error, QVariantMap result) {
        // FIXME: to remain backwards-compatible the signature here is "callback(data, n/a)"
        jsCallback(handler, result.value("data").toString(), { { "errorMessage", error } });
    });

    connect(assetRequest, &AssetRequest::finished, assetRequest, [deferred](AssetRequest* request) {
        Q_ASSERT(QThread::currentThread() == request->thread());
        // we are now on the "Resource Manager" thread
        Q_ASSERT(request->getState() == AssetRequest::Finished);

        if (request->getError() == AssetRequest::Error::NoError) {
            QString data = QString::fromUtf8(request->getData());
            // forward a thread-safe values back to our thread
            deferred->resolve({ { "data", data } });
        } else {
            // FIXME: propagate error to scripts? (requires changing signature or inverting param order above..)
            //deferred->resolve(request->getErrorString(), { { "error", requet->getError() } });
            qCDebug(scriptengine) << "AssetScriptingInterface::downloadData ERROR: " << request->getErrorString();
        }

        request->deleteLater();
    });

    assetRequest->start();
}

void AssetScriptingInterface::setBakingEnabled(QString path, bool enabled, QScriptValue callback) {
    auto setBakingEnabledRequest = DependencyManager::get<AssetClient>()->createSetBakingEnabledRequest({ path }, enabled);

    Promise deferred = jsPromiseReady(makePromise(__FUNCTION__), thisObject(), callback);

    connect(setBakingEnabledRequest, &SetBakingEnabledRequest::finished, setBakingEnabledRequest, [deferred](SetBakingEnabledRequest* request) {
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
    auto handler = jsBindCallback(thisObject(), callback);
    JS_VERIFY(AssetUtils::isValidFilePath(path), "invalid ATP file path: " + asset + "(path:"+path+")");
    JS_VERIFY(callback.isFunction(), "expected second parameter to be a callback function");
    Promise promise = getAssetInfo(path);
    promise->ready([=](QString error, QVariantMap result) {
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
        qCDebug(scriptengine) << "WARNING -- jsVerify failed outside of a valid JS context: " + error;
    }
    return false;
}

QScriptValue AssetScriptingInterface::jsBindCallback(QScriptValue scope, QScriptValue callback) {
    QScriptValue handler = ::makeScopedHandlerObject(scope, callback);
    QScriptValue value = handler.property("callback");
    if (!jsVerify(handler.isObject() && value.isFunction(),
                 QString("jsBindCallback -- .callback is not a function (%1)").arg(value.toVariant().typeName()))) {
        return QScriptValue();
    }
    return handler;
}

Promise AssetScriptingInterface::jsPromiseReady(Promise promise, QScriptValue scope, QScriptValue callback) {
    auto handler = jsBindCallback(scope, callback);
    if (!jsVerify(handler.isValid(), "jsPromiseReady -- invalid callback handler")) {
        return nullptr;
    }
    return promise->ready([this, handler](QString error, QVariantMap result) {
        jsCallback(handler, error, result);
    });
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

/**jsdoc
 * @typedef {string} Assets.GetOptions.ResponseType
 * <p>Available <code>responseType</code> values for use with @{link Assets.getAsset} and @{link Assets.loadFromCache} configuration option. </p>
 * <table>
 *   <thead>
 *     <tr><th>responseType</th><th>typeof response value</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>"text"</code></td><td>contents returned as utf-8 decoded <code>String</code> value</td></tr>
 *     <tr><td><code>"arraybuffer"</code></td><td>contents as a binary <code>ArrayBuffer</code> object</td></tr>
 *     <tr><td><code>"json"</code></td><td>contents as a parsed <code>JSON</code> object</td></tr>
 *   </tbody>
 * </table>
 */

void AssetScriptingInterface::getAsset(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    JS_VERIFY(options.isObject() || options.isString(), "expected request options Object or URL as first parameter");

    auto decompress = options.property("decompress").toBool() || options.property("compressed").toBool();
    auto responseType = options.property("responseType").toString().toLower();
    auto url = options.property("url").toString();
    if (options.isString()) {
        url = options.toString();
    }
    if (responseType.isEmpty()) {
        responseType = "text";
    }
    auto asset = AssetUtils::getATPUrl(url).path();
    JS_VERIFY(AssetUtils::isValidHash(asset) || AssetUtils::isValidFilePath(asset),
              QString("Invalid ATP url '%1'").arg(url));
    JS_VERIFY(RESPONSE_TYPES.contains(responseType),
              QString("Invalid responseType: '%1' (expected: %2)").arg(responseType).arg(RESPONSE_TYPES.join(" | ")));

    Promise fetched = jsPromiseReady(makePromise("fetched"), scope, callback);
    Promise mapped = makePromise("mapped");

    mapped->fail(fetched);
    mapped->then([=](QVariantMap result) {
        QString hash = result.value("hash").toString();
        QString url = result.value("url").toString();
        if (!AssetUtils::isValidHash(hash)) {
            fetched->reject("internal hash error: " + hash, result);
        } else {
            Promise promise = loadAsset(hash, decompress, responseType);
            promise->mixin(result);
            promise->ready([=](QString error, QVariantMap loadResult) {
                loadResult["url"] = url; // maintain mapped .url in results (vs. atp:hash returned by loadAsset)
                fetched->handle(error, loadResult);
            });
        }
    });

    if (AssetUtils::isValidHash(asset)) {
        mapped->resolve({
            { "hash", asset },
            { "url", url },
        });
    } else {
        getAssetInfo(asset)->ready(mapped);
    }
}

void AssetScriptingInterface::resolveAsset(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    const QString& URL{ "url" };

    auto url = (options.isString() ? options : options.property(URL)).toString();
    auto asset = AssetUtils::getATPUrl(url).path();

    JS_VERIFY(AssetUtils::isValidFilePath(asset) || AssetUtils::isValidHash(asset),
             "expected options to be an asset URL or request options containing .url property");

    jsPromiseReady(getAssetInfo(asset), scope, callback);
}

void AssetScriptingInterface::decompressData(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    auto data = options.property("data");
    QByteArray dataByteArray = qscriptvalue_cast<QByteArray>(data);
    auto responseType = options.property("responseType").toString().toLower();
    if (responseType.isEmpty()) {
        responseType = "text";
    }
    Promise completed = jsPromiseReady(makePromise(__FUNCTION__), scope, callback);
    Promise decompressed = decompressBytes(dataByteArray);
    if (responseType == "arraybuffer") {
        decompressed->ready(completed);
    } else {
        decompressed->ready([=](QString error, QVariantMap result) {
            Promise converted = convertBytes(result.value("data").toByteArray(), responseType);
            converted->mixin(result);
            converted->ready(completed);
        });
    }
}

namespace {
    const int32_t DEFAULT_GZIP_COMPRESSION_LEVEL = -1;
    const int32_t MAX_GZIP_COMPRESSION_LEVEL = 9;
}
void AssetScriptingInterface::compressData(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    auto data = options.property("data").isValid() ? options.property("data") : options;
    QByteArray dataByteArray = data.isString() ? data.toString().toUtf8() : qscriptvalue_cast<QByteArray>(data);
    int level = options.property("level").isNumber() ? options.property("level").toInt32() : DEFAULT_GZIP_COMPRESSION_LEVEL;
    JS_VERIFY(level >= DEFAULT_GZIP_COMPRESSION_LEVEL || level <= MAX_GZIP_COMPRESSION_LEVEL, QString("invalid .level %1").arg(level));
    jsPromiseReady(compressBytes(dataByteArray, level), scope, callback);
}

void AssetScriptingInterface::putAsset(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    auto compress = options.property("compress").toBool() || options.property("compressed").toBool();
    auto data = options.isObject() ? options.property("data") : options;
    auto rawPath = options.property("path").toString();
    auto path = AssetUtils::getATPUrl(rawPath).path();

    QByteArray dataByteArray = data.isString() ? data.toString().toUtf8() : qscriptvalue_cast<QByteArray>(data);

    JS_VERIFY(path.isEmpty() || AssetUtils::isValidFilePath(path),
              QString("expected valid ATP file path '%1' ('%2')").arg(rawPath).arg(path));
    JS_VERIFY(dataByteArray.size() > 0,
              QString("expected non-zero .data (got %1 / #%2 bytes)").arg(data.toVariant().typeName()).arg(dataByteArray.size()));

    // [compressed] => uploaded to server => [mapped to path]
    Promise prepared = makePromise("putAsset::prepared");
    Promise uploaded = makePromise("putAsset::uploaded");
    Promise completed = makePromise("putAsset::completed");
    jsPromiseReady(completed, scope, callback);

    if (compress) {
        Promise compress = compressBytes(dataByteArray, DEFAULT_GZIP_COMPRESSION_LEVEL);
        compress->ready(prepared);
    } else {
        prepared->resolve({{ "data", dataByteArray }});
    }

    prepared->fail(completed);
    prepared->then([=](QVariantMap result) {
        Promise upload = uploadBytes(result.value("data").toByteArray());
        upload->mixin(result);
        upload->ready(uploaded);
    });

    uploaded->fail(completed);
    if (path.isEmpty()) {
        uploaded->then(completed);
    } else {
        uploaded->then([=](QVariantMap result) {
            QString hash = result.value("hash").toString();
            if (!AssetUtils::isValidHash(hash)) {
                completed->reject("path mapping requested, but did not receive valid hash", result);
            } else {
                Promise link = symlinkAsset(hash, path);
                link->mixin(result);
                link->ready(completed);
            }
        });
    }
}

void AssetScriptingInterface::queryCacheMeta(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    QString url = options.isString() ? options.toString() : options.property("url").toString();
    JS_VERIFY(QUrl(url).isValid(), QString("Invalid URL '%1'").arg(url));
    jsPromiseReady(Parent::queryCacheMeta(url), scope, callback);
}

void AssetScriptingInterface::loadFromCache(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    QString url, responseType;
    bool decompress = false;
    if (options.isString()) {
        url = options.toString();
        responseType = "text";
    } else {
        url = options.property("url").toString();
        responseType = options.property("responseType").isValid() ? options.property("responseType").toString() : "text";
        decompress = options.property("decompress").toBool() || options.property("compressed").toBool();
    }
    JS_VERIFY(QUrl(url).isValid(), QString("Invalid URL '%1'").arg(url));
    JS_VERIFY(RESPONSE_TYPES.contains(responseType),
              QString("Invalid responseType: '%1' (expected: %2)").arg(responseType).arg(RESPONSE_TYPES.join(" | ")));

    jsPromiseReady(Parent::loadFromCache(url, decompress, responseType), scope, callback);
}

bool AssetScriptingInterface::canWriteCacheValue(const QUrl& url) {
    auto scriptEngine = qobject_cast<ScriptEngine*>(engine());
    if (!scriptEngine) {
        return false;
    }
    // allow cache writes only from Client, EntityServer and Agent scripts
    bool isAllowedContext = (
        scriptEngine->isClientScript() ||
        scriptEngine->isAgentScript()
    );
    if (!isAllowedContext) {
        return false;
    }
    return true;
}

void AssetScriptingInterface::saveToCache(QScriptValue options, QScriptValue scope, QScriptValue callback) {
    JS_VERIFY(options.isObject(), QString("expected options object as first parameter not: %1").arg(options.toVariant().typeName()));

    QString url = options.property("url").toString();
    QByteArray data = qscriptvalue_cast<QByteArray>(options.property("data"));
    QVariantMap headers = qscriptvalue_cast<QVariantMap>(options.property("headers"));

    saveToCache(url, data, headers, scope, callback);
}

void AssetScriptingInterface::saveToCache(const QUrl& rawURL, const QByteArray& data, const QVariantMap& metadata, QScriptValue scope, QScriptValue callback) {
    QUrl url = rawURL;
    if (url.path().isEmpty() && !data.isEmpty()) {
        // generate a valid ATP URL from the data  -- appending any existing fragment or querystring values
        auto atpURL = AssetUtils::getATPUrl(hashDataHex(data));
        atpURL.setQuery(url.query());
        atpURL.setFragment(url.fragment());
        url = atpURL;
    }
    auto hash = AssetUtils::extractAssetHash(url.toDisplayString());

    JS_VERIFY(url.isValid(), QString("Invalid URL '%1'").arg(url.toString()));
    JS_VERIFY(canWriteCacheValue(url), "Invalid cache write URL: " + url.toString());
    JS_VERIFY(url.scheme() == "atp" || url.scheme() == "cache", "only 'atp' and 'cache' URL schemes supported");
    JS_VERIFY(hash.isEmpty() || hash == hashDataHex(data), QString("invalid checksum hash for atp:HASH style URL (%1 != %2)").arg(hash, hashDataHex(data)));


    jsPromiseReady(Parent::saveToCache(url, data, metadata), scope, callback);
}
