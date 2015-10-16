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

#include <QtCore/QByteArray>
#include <QtCore/QUrl>

using MessageID = uint32_t;
using DataOffset = int64_t;

const size_t SHA256_HASH_LENGTH = 32;
const size_t SHA256_HASH_HEX_LENGTH = 64;
const uint64_t MAX_UPLOAD_SIZE = 1000 * 1000 * 1000; // 1GB

enum AssetServerError : uint8_t {
    NoError = 0,
    AssetNotFound,
    InvalidByteRange,
    AssetTooLarge,
    PermissionDenied
};

QUrl getATPUrl(const QString& hash, const QString& extension = QString());

QByteArray hashData(const QByteArray& data);

QByteArray loadFromCache(const QUrl& url);
bool saveToCache(const QUrl& url, const QByteArray& file);

#endif
