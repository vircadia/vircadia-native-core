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

#include <QtCore/QCryptographicHash>

using MessageID = uint32_t;
using DataOffset = int64_t;

const size_t SHA256_HASH_LENGTH = 32;
const size_t SHA256_HASH_HEX_LENGTH = 64;
const uint64_t MAX_UPLOAD_SIZE = 1000 * 1000 * 1000; // 1GB

enum AssetServerError : uint8_t {
    NoError,
    AssetNotFound,
    InvalidByteRange,
    AssetTooLarge,
    PermissionDenied
};

const QString ATP_SCHEME = "atp";

inline QByteArray hashData(const QByteArray& data) { return QCryptographicHash::hash(data, QCryptographicHash::Sha256); }

#endif
