//
//  UUIDHasher.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-11-05.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UUIDHasher_h
#define hifi_UUIDHasher_h

#include "UUID.h"

class UUIDHasher {
public:
    size_t operator()(const QUuid& uuid) const {
        return uuid.data1 ^ uuid.data2 ^ (uuid.data3 << 16)
            ^ ((uuid.data4[0] << 24) | (uuid.data4[1] << 16) | (uuid.data4[2] << 8) | uuid.data4[3])
            ^ ((uuid.data4[4] << 24) | (uuid.data4[5] << 16) | (uuid.data4[6] << 8) | uuid.data4[7]);
    }
};

#endif // hifi_UUIDHasher_h