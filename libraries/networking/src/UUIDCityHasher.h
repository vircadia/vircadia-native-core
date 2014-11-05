//
//  UUIDCityHasher.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-11-05.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UUIDCityHasher_h
#define hifi_UUIDCityHasher_h

#include <libcuckoo/city.h>

#include "UUID.h"

class UUIDCityHasher {
public:
    size_t operator()(const QUuid& key) const {
        return CityHash64(key.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
    }
};

#endif // hifi_UUIDCityHasher_h