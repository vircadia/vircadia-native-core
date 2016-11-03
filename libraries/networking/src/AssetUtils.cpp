//
//  AssetUtils.h
//  libraries/networking/src
//
//  Created by Clément Brisset on 10/12/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetUtils.h"

#include <memory>

#include <QtCore/QCryptographicHash>
#include <QtNetwork/QAbstractNetworkCache>

#include "NetworkAccessManager.h"
#include "NetworkLogging.h"

#include "ResourceManager.h"

QUrl getATPUrl(const QString& hash) {
    return QUrl(QString("%1:%2").arg(URL_SCHEME_ATP, hash));
}

QByteArray hashData(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QByteArray loadFromCache(const QUrl& url) {
    if (auto cache = NetworkAccessManager::getInstance().cache()) {

        // caller is responsible for the deletion of the ioDevice, hence the unique_ptr
        if (auto ioDevice = std::unique_ptr<QIODevice>(cache->data(url))) {
            qCDebug(asset_client) << url.toDisplayString() << "loaded from disk cache.";
            return ioDevice->readAll();
        } else {
            qCDebug(asset_client) << url.toDisplayString() << "not in disk cache";
        }

    } else {
        qCWarning(asset_client) << "No disk cache to load assets from.";
    }
    return QByteArray();
}

bool saveToCache(const QUrl& url, const QByteArray& file) {
    if (auto cache = NetworkAccessManager::getInstance().cache()) {
        if (!cache->metaData(url).isValid()) {
            QNetworkCacheMetaData metaData;
            metaData.setUrl(url);
            metaData.setSaveToDisk(true);
            metaData.setLastModified(QDateTime::currentDateTime());
            metaData.setExpirationDate(QDateTime()); // Never expires

            // ioDevice is managed by the cache and should either be passed back to insert or remove!
            if (auto ioDevice = cache->prepare(metaData)) {
                ioDevice->write(file);
                cache->insert(ioDevice);
                qCDebug(asset_client) << url.toDisplayString() << "saved to disk cache";
                return true;
            }
            qCWarning(asset_client) << "Could not save" << url.toDisplayString() << "to disk cache.";
        }
    } else {
        qCWarning(asset_client) << "No disk cache to save assets to.";
    }
    return false;
}

bool isValidFilePath(const AssetPath& filePath) {
    QRegExp filePathRegex { ASSET_FILE_PATH_REGEX_STRING };
    return filePathRegex.exactMatch(filePath);
}

bool isValidPath(const AssetPath& path) {
    QRegExp pathRegex { ASSET_PATH_REGEX_STRING };
    return pathRegex.exactMatch(path);
}

bool isValidHash(const AssetHash& hash) {
    QRegExp hashRegex { ASSET_HASH_REGEX_STRING };
    return hashRegex.exactMatch(hash);
}
