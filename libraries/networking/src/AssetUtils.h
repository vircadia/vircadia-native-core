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

#include "NLPacketList.h"

using MessageID = uint32_t;
using DataOffset = int64_t;

const size_t HASH_HEX_LENGTH = 64;
const uint64_t MAX_UPLOAD_SIZE = 1000 * 1000 * 1000; // 1GB

enum AssetServerError : uint8_t {
    NO_ERROR = 0,
    ASSET_NOT_FOUND,
    INVALID_BYTE_RANGE,
    ASSET_TOO_LARGE,
};

#endif
