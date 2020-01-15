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
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo> // for baseName
#include <QtNetwork/QAbstractNetworkCache>

#include "NetworkAccessManager.h"
#include "NetworkLogging.h"
#include "NetworkingConstants.h"
#include "MetaverseAPI.h"

#include "ResourceManager.h"

namespace AssetUtils {

// Extract the valid AssetHash portion from atp: URLs like "[atp:]HASH[.fbx][?query]"
// (or an invalid AssetHash if not found)
AssetHash extractAssetHash(const QString& input) {
    if (isValidHash(input)) {
        return input;
    }
    QString path = getATPUrl(input).path();
    QString baseName = QFileInfo(path).baseName();
    if (isValidHash(baseName)) {
        return baseName;
    }
    return AssetHash();
}

// Get the normalized ATP URL for a raw hash, /path or "atp:" input string.
QUrl getATPUrl(const QString& input) {
    QUrl url = input;
    if (!url.scheme().isEmpty() && url.scheme() != URL_SCHEME_ATP) {
        return QUrl();
    }
    // this strips extraneous info from the URL (while preserving fragment/querystring)
    QString path = url.toEncoded(
        QUrl::RemoveAuthority | QUrl::RemoveScheme |
        QUrl::StripTrailingSlash | QUrl::NormalizePathSegments
    );
    QString baseName = QFileInfo(url.path()).baseName();
    if (isValidPath(path) || isValidHash(baseName)) {
        return QUrl(QString("%1:%2").arg(URL_SCHEME_ATP).arg(path));
    }
    return QUrl();
}

QByteArray hashData(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QByteArray loadFromCache(const QUrl& url) {
    if (auto cache = NetworkAccessManager::getInstance().cache()) {

        // caller is responsible for the deletion of the ioDevice, hence the unique_ptr
        if (auto ioDevice = std::unique_ptr<QIODevice>(cache->data(url))) {
            return ioDevice->readAll();
        }

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
                return true;
            }
        }
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

QString bakingStatusToString(BakingStatus status) {
    switch (status) {
        case NotBaked:
            return "Not Baked";
        case Pending:
            return "Pending";
        case Baking:
            return "Baking";
        case Baked:
            return "Baked";
        case Error:
            return "Error";
        default:
            return "--";
    }
}

} // namespace AssetUtils
