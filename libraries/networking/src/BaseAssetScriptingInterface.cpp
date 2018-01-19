//
//  BaseAssetScriptingInterface.cpp
//  libraries/networking/src
//
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include "BaseAssetScriptingInterface.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QMimeDatabase>
#include <QThread>

#include "AssetRequest.h"
#include "AssetUpload.h"
#include "AssetUtils.h"
#include "MappingRequest.h"
#include "NetworkLogging.h"

#include <RegisteredMetaTypes.h>

#include <shared/QtHelpers.h>
#include "Gzip.h"

using Promise = MiniPromise::Promise;

QSharedPointer<AssetClient> BaseAssetScriptingInterface::assetClient() {
    return DependencyManager::get<AssetClient>();
}

BaseAssetScriptingInterface::BaseAssetScriptingInterface(QObject* parent) : QObject(parent) {}

bool BaseAssetScriptingInterface::initializeCache() {
    auto assets = assetClient();
    if (!assets) {
        return false; // not yet possible to initialize the cache
    }
    if (!_cacheDirectory.isEmpty()) {
        return true; // cache is ready
    }

    // attempt to initialize the cache
    QMetaObject::invokeMethod(assets.data(), "init");

    Promise deferred = makePromise("BaseAssetScriptingInterface--queryCacheStatus");
    deferred->then([&](QVariantMap result) {
        _cacheDirectory = result.value("cacheDirectory").toString();
    });
    deferred->fail([&](QString error) {
        qDebug() << "BaseAssetScriptingInterface::queryCacheStatus ERROR" << QThread::currentThread() << error;
    });
    assets->cacheInfoRequest(deferred);
    return false; // cache is not ready yet
}

Promise BaseAssetScriptingInterface::getCacheStatus() {
    Promise deferred = makePromise(__FUNCTION__);
    DependencyManager::get<AssetClient>()->cacheInfoRequest(deferred);
    return deferred;
}

Promise BaseAssetScriptingInterface::queryCacheMeta(const QUrl& url) {
    Promise deferred = makePromise(__FUNCTION__);
    DependencyManager::get<AssetClient>()->queryCacheMeta(deferred, url);
    return deferred;
}

Promise BaseAssetScriptingInterface::loadFromCache(const QUrl& url) {
    Promise deferred = makePromise(__FUNCTION__);
    DependencyManager::get<AssetClient>()->loadFromCache(deferred, url);
    return deferred;
}

Promise BaseAssetScriptingInterface::saveToCache(const QUrl& url, const QByteArray& data, const QVariantMap& headers) {
    Promise deferred = makePromise(__FUNCTION__);
    DependencyManager::get<AssetClient>()->saveToCache(deferred, url, data, headers);
    return deferred;
}

Promise BaseAssetScriptingInterface::loadAsset(QString asset, bool decompress, QString responseType) {
    auto hash = AssetUtils::extractAssetHash(asset);
    auto url = AssetUtils::getATPUrl(hash).toString();

    QVariantMap metaData = {
        { "_asset", asset },
        { "_type", "download" },
        { "hash", hash },
        { "url", url },
        { "responseType", responseType },
    };

    Promise fetched = makePromise("loadAsset::fetched"),
        loaded = makePromise("loadAsset::loaded");

    downloadBytes(hash)
        ->mixin(metaData)
        ->ready([=](QString error, QVariantMap result) {
        Q_ASSERT(thread() == QThread::currentThread());
        fetched->mixin(result);
        if (decompress) {
            decompressBytes(result.value("data").toByteArray())
                ->mixin(result)
                ->ready([=](QString error, QVariantMap result) {
                    fetched->handle(error, result);
                });
        } else {
            fetched->handle(error, result);
        }
    });

    fetched->ready([=](QString error, QVariantMap result) {
        if (responseType == "arraybuffer") {
            loaded->resolve(NoError, result);
        } else {
            convertBytes(result.value("data").toByteArray(), responseType)
                ->mixin(result)
                ->ready([=](QString error, QVariantMap result) {
                loaded->resolve(NoError, result);
            });
        }
    });

    return loaded;
}

Promise BaseAssetScriptingInterface::convertBytes(const QByteArray& dataByteArray, const QString& responseType) {
    QVariantMap result;
    Promise conversion = makePromise(__FUNCTION__);
    if (dataByteArray.size() == 0) {
        result["response"] = QString();
    } else if (responseType == "text") {
        result["response"] = QString::fromUtf8(dataByteArray);
    } else if (responseType == "json") {
        QJsonParseError status;
        auto parsed = QJsonDocument::fromJson(dataByteArray, &status);
        if (status.error == QJsonParseError::NoError) {
            result["response"] = parsed.isArray() ?
                QVariant(parsed.array().toVariantList()) :
                QVariant(parsed.object().toVariantMap());
        } else {
            QVariantMap errorResult = {
                { "error", status.error },
                { "offset", status.offset },
            };
            return conversion->reject("JSON Parse Error: " + status.errorString(), errorResult);
        }
    } else if (responseType == "arraybuffer") {
        result["response"] = dataByteArray;
    }
    return conversion->resolve(NoError, result);
}

Promise BaseAssetScriptingInterface::decompressBytes(const QByteArray& dataByteArray) {
    QByteArray inflated;
    auto start = usecTimestampNow();
    if (gunzip(dataByteArray, inflated)) {
        auto end = usecTimestampNow();
        return makePromise(__FUNCTION__)->resolve(NoError, {
            { "_compressedByteLength", dataByteArray.size() },
            { "_compressedContentType", QMimeDatabase().mimeTypeForData(dataByteArray).name() },
            { "_compressMS", (double)(end - start) / 1000.0 },
            { "decompressed", true },
            { "byteLength", inflated.size() },
            { "contentType", QMimeDatabase().mimeTypeForData(inflated).name() },
            { "data", inflated },
       });
    } else {
        return makePromise(__FUNCTION__)->reject("gunzip error", {});
    }
}

Promise BaseAssetScriptingInterface::compressBytes(const QByteArray& dataByteArray, int level) {
    QByteArray deflated;
    auto start = usecTimestampNow();
    if (gzip(dataByteArray, deflated, level)) {
        auto end = usecTimestampNow();
        return makePromise(__FUNCTION__)->resolve(NoError, {
            { "_uncompressedByteLength", dataByteArray.size() },
            { "_uncompressedContentType", QMimeDatabase().mimeTypeForData(dataByteArray).name() },
            { "_compressMS", (double)(end - start) / 1000.0 },
            { "compressed", true },
            { "byteLength", deflated.size() },
            { "contentType", QMimeDatabase().mimeTypeForData(deflated).name() },
            { "data", deflated },
       });
    } else {
        return makePromise(__FUNCTION__)->reject("gzip error", {});
    }
}

Promise BaseAssetScriptingInterface::downloadBytes(QString hash) {
    auto assetClient = DependencyManager::get<AssetClient>();
    QPointer<AssetRequest> assetRequest = assetClient->createRequest(hash);
    Promise deferred = makePromise(__FUNCTION__);

    QObject::connect(assetRequest, &AssetRequest::finished, assetRequest, [this, deferred](AssetRequest* request) {
        // note: we are now on the "Resource Manager" thread
        Q_ASSERT(QThread::currentThread() == request->thread());
        Q_ASSERT(request->getState() == AssetRequest::Finished);
        QString error;
        QVariantMap result;
        if (request->getError() == AssetRequest::Error::NoError) {
            QByteArray data = request->getData();
            result = {
                { "url", request->getUrl() },
                { "hash", request->getHash() },
                { "cached", request->loadedFromCache() },
                { "content-type", QMimeDatabase().mimeTypeForData(data).name() },
                { "data", data },
            };
        } else {
            error = request->getError();
            result = { { "error", request->getError() } };
        }
        // forward thread-safe copies back to our thread
        deferred->handle(error, result);
        request->deleteLater();
    });
    assetRequest->start();
    return deferred;
}

Promise BaseAssetScriptingInterface::uploadBytes(const QByteArray& bytes) {
    Promise deferred = makePromise(__FUNCTION__);
    QPointer<AssetUpload> upload = DependencyManager::get<AssetClient>()->createUpload(bytes);

    QObject::connect(upload, &AssetUpload::finished, upload, [=](AssetUpload* upload, const QString& hash) {
        Q_ASSERT(QThread::currentThread() == upload->thread());
        // note: we are now on the "Resource Manager" thread
        QString error;
        QVariantMap result;
        if (upload->getError() == AssetUpload::NoError) {
            result = {
                { "hash", hash },
                { "url", AssetUtils::getATPUrl(hash).toString() },
                { "filename", upload->getFilename() },
            };
        } else {
            error = upload->getErrorString();
            result = { { "error", upload->getError() } };
        }
        // forward thread-safe copies back to our thread
        deferred->handle(error, result);
        upload->deleteLater();
    });
    upload->start();
    return deferred;
}

Promise BaseAssetScriptingInterface::getAssetInfo(QString asset) {
    auto deferred = makePromise(__FUNCTION__);
    auto url = AssetUtils::getATPUrl(asset);
    auto path = url.path();
    auto hash = AssetUtils::extractAssetHash(asset);
    if (AssetUtils::isValidHash(hash)) {
        // already a valid ATP hash -- nothing to do
        deferred->resolve(NoError, {
            { "hash", hash },
            { "path", path },
            { "url", url },
        });
    } else if (AssetUtils::isValidFilePath(path)) {
        auto assetClient = DependencyManager::get<AssetClient>();
        QPointer<GetMappingRequest> request = assetClient->createGetMappingRequest(path);

        QObject::connect(request, &GetMappingRequest::finished, request, [=]() {
            Q_ASSERT(QThread::currentThread() == request->thread());
            // note: we are now on the "Resource Manager" thread
            QString error;
            QVariantMap result;
            if (request->getError() == GetMappingRequest::NoError) {
                result = {
                    { "_hash", hash },
                    { "_path", path },
                    { "_url", url },
                    { "hash", request->getHash() },
                    { "wasRedirected", request->wasRedirected() },
                    { "path", request->wasRedirected() ? request->getRedirectedPath() : path },
                };
            } else {
                error = request->getErrorString();
                result = { { "error", request->getError() } };
            }
            // forward thread-safe copies back to our thread
            deferred->handle(error, result);
            request->deleteLater();
        });
        request->start();
    } else {
        deferred->reject("invalid ATP file path: " + asset + "("+path+")", {});
    }
    return deferred;
}

Promise BaseAssetScriptingInterface::symlinkAsset(QString hash, QString path) {
    auto deferred = makePromise(__FUNCTION__);
    auto assetClient = DependencyManager::get<AssetClient>();
    QPointer<SetMappingRequest> setMappingRequest = assetClient->createSetMappingRequest(path, hash);

    connect(setMappingRequest, &SetMappingRequest::finished, setMappingRequest, [=](SetMappingRequest* request) {
        Q_ASSERT(QThread::currentThread() == request->thread());
        // we are now on the "Resource Manager" thread
        QString error;
        QVariantMap result;
        if (request->getError() == SetMappingRequest::NoError) {
            result = {
                { "_hash", hash },
                { "_path", path },
                { "hash", request->getHash() },
                { "path", request->getPath() },
                { "url", AssetUtils::getATPUrl(request->getPath()).toString() },
            };
        } else {
            error = request->getErrorString();
            result = { { "error", request->getError() } };
        }
        // forward results back to our thread
        deferred->handle(error, result);
        request->deleteLater();
    });
    setMappingRequest->start();
    return deferred;
}
