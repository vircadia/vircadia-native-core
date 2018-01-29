//
//  AssetUtils.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssetUtils_h
#define hifi_AssetUtils_h

#include <cstdint>

#include <map>

#include <QtCore/QByteArray>
#include <QtCore/QUrl>

namespace AssetUtils {

using DataOffset = int64_t;

using AssetPath = QString;
using AssetHash = QString;
using AssetPathList = QStringList;

const size_t SHA256_HASH_LENGTH = 32;
const size_t SHA256_HASH_HEX_LENGTH = 64;
const uint64_t MAX_UPLOAD_SIZE = 1000 * 1000 * 1000; // 1GB

const QString ASSET_FILE_PATH_REGEX_STRING = "^(\\/[^\\/\\0]+)+$";
const QString ASSET_PATH_REGEX_STRING = "^\\/([^\\/\\0]+(\\/)?)+$";
const QString ASSET_HASH_REGEX_STRING = QString("^[a-fA-F0-9]{%1}$").arg(SHA256_HASH_HEX_LENGTH);

const QString HIDDEN_BAKED_CONTENT_FOLDER = "/.baked/";

enum AssetServerError : uint8_t {
    NoError = 0,
    AssetNotFound,
    InvalidByteRange,
    AssetTooLarge,
    PermissionDenied,
    MappingOperationFailed,
    FileOperationFailed
};

enum AssetMappingOperationType : uint8_t {
    Get = 0,
    GetAll,
    Set,
    Delete,
    Rename,
    SetBakingEnabled
};

enum BakingStatus {
    Irrelevant,
    NotBaked,
    Pending,
    Baking,
    Baked,
    Error
};

struct MappingInfo {
    AssetHash hash;
    BakingStatus status;
    QString bakingErrors;
};

using AssetMapping = std::map<AssetPath, MappingInfo>;

QUrl getATPUrl(const QString& input);
AssetHash extractAssetHash(const QString& input);

QByteArray hashData(const QByteArray& data);

QByteArray loadFromCache(const QUrl& url);
bool saveToCache(const QUrl& url, const QByteArray& file);

bool isValidFilePath(const AssetPath& path);
bool isValidPath(const AssetPath& path);
bool isValidHash(const QString& hashString);

QString bakingStatusToString(BakingStatus status);

} // namespace AssetUtils

#endif // hifi_AssetUtils_h
