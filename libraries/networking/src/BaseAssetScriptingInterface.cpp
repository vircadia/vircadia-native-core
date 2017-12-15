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

BaseAssetScriptingInterface::BaseAssetScriptingInterface(QObject* parent) : QObject(parent), _cache(this) {
}


bool BaseAssetScriptingInterface::initializeCache() {
    qDebug() << "BaseAssetScriptingInterface::getCacheStatus -- current values" << _cache.cacheDirectory() << _cache.cacheSize() << _cache.maximumCacheSize();

    // NOTE: *instances* of QNetworkDiskCache are not thread-safe -- however, different threads can effectively
    // use the same underlying cache if configured with identical settings.  Once AssetClient's disk cache settings
    // become available we configure our instance to match.
    auto assets = assetClient();
    if (!assets) {
        return false; // not yet possible to initialize the cache
    }
    if (_cache.cacheDirectory().size()) {
        return true; // cache is ready
    }

    // attempt to initialize the cache
    qDebug() << "BaseAssetScriptingInterface::getCacheStatus -- invoking AssetClient::init" << assetClient().data();
    QMetaObject::invokeMethod(assetClient().data(), "init");

    qDebug() << "BaseAssetScriptingInterface::getCacheStatus querying cache status" << QThread::currentThread();
    Promise deferred = makePromise("BaseAssetScriptingInterface--queryCacheStatus");
    deferred->then([&](QVariantMap result) {
        qDebug() << "//queryCacheStatus" << QThread::currentThread();
        auto cacheDirectory = result.value("cacheDirectory").toString();
        auto cacheSize = result.value("cacheSize").toLongLong();
        auto maximumCacheSize = result.value("maximumCacheSize").toLongLong();
        qDebug() << "///queryCacheStatus" << cacheDirectory << cacheSize << maximumCacheSize;
        _cache.setCacheDirectory(cacheDirectory);
        _cache.setMaximumCacheSize(maximumCacheSize);
    });
    deferred->fail([&](QString error) {
        qDebug() << "//queryCacheStatus ERROR" << QThread::currentThread() << error;
    });
    assets->cacheInfoRequest(deferred);
    return false; // cache is not ready yet
}

QVariantMap BaseAssetScriptingInterface::getCacheStatus() {
    //assetClient()->cacheInfoRequest(this, "onCacheInfoResponse");
    return {
        { "cacheDirectory", _cache.cacheDirectory() },
        { "cacheSize", _cache.cacheSize() },
        { "maximumCacheSize", _cache.maximumCacheSize() },
    };
}

QVariantMap BaseAssetScriptingInterface::queryCacheMeta(const QUrl& url) {
    QNetworkCacheMetaData metaData = _cache.metaData(url);
    QVariantMap attributes, rawHeaders;

    QHashIterator<QNetworkRequest::Attribute, QVariant> i(metaData.attributes());
    while (i.hasNext()) {
        i.next();
        attributes[QString::number(i.key())] = i.value();
    }
    for (const auto& i : metaData.rawHeaders()) {
        rawHeaders[i.first] = i.second;
    }
    return {
        { "isValid", metaData.isValid() },
        { "url", metaData.url() },
        { "expirationDate", metaData.expirationDate() },
        { "lastModified", metaData.lastModified().toString().isEmpty() ? QDateTime() : metaData.lastModified() },
        { "saveToDisk", metaData.saveToDisk() },
        { "attributes", attributes },
        { "rawHeaders", rawHeaders },
    };
}

QVariantMap BaseAssetScriptingInterface::loadFromCache(const QUrl& url) {
    qDebug() << "loadFromCache" << url;
    QVariantMap result = {
        { "metadata", queryCacheMeta(url) },
        { "data", QByteArray() },
    };
    // caller is responsible for the deletion of the ioDevice, hence the unique_ptr
    if (auto ioDevice = std::unique_ptr<QIODevice>(_cache.data(url))) {
        QByteArray data = ioDevice->readAll();
        qCDebug(asset_client) << url.toDisplayString() << "loaded from disk cache (" << data.size() << " bytes)";
        result["data"] = data;
    }
    return result;
}

namespace {
    // parse RFC 1123 HTTP date format
    QDateTime parseHttpDate(const QString& dateString) {
        QDateTime dt = QDateTime::fromString(dateString.left(25), "ddd, dd MMM yyyy HH:mm:ss");
        dt.setTimeSpec(Qt::UTC);
        return dt;
    }
}

bool BaseAssetScriptingInterface::saveToCache(const QUrl& url, const QByteArray& data, const QVariantMap& headers) {
    QDateTime lastModified = headers.contains("last-modified") ?
        parseHttpDate(headers["last-modified"].toString()) :
        QDateTime::currentDateTimeUtc();
    QDateTime expirationDate = headers.contains("expires") ?
        parseHttpDate(headers["expires"].toString()) :
        QDateTime(); // never expires
    QNetworkCacheMetaData metaData;
    metaData.setUrl(url);
    metaData.setSaveToDisk(true);
    metaData.setLastModified(lastModified);
    metaData.setExpirationDate(expirationDate);
    if (auto ioDevice = _cache.prepare(metaData)) {
        ioDevice->write(data);
        _cache.insert(ioDevice);
        qCDebug(asset_client) << url.toDisplayString() << "saved to disk cache ("<< data.size()<<" bytes)";
        return true;
    }
    qCWarning(asset_client) << "Could not save" << url.toDisplayString() << "to disk cache.";
    return false;
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
            qDebug() << "loadAsset::decompressBytes...";
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
        qDebug() << "loadAsset::fetched" << error;
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
        qDebug() << "...BaseAssetScriptingInterface::downloadBytes" << request->getErrorString();
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
        qDebug() << "//BaseAssetScriptingInterface::downloadBytes" << error << result.keys();
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
