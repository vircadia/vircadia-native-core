//
//  ColorChannelMapping.h
//  libraries/shared/src
//
//  Created by Sabrina Shanman on 2019/02/05.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ColorChannelMapping_h
#define hifi_ColorChannelMapping_h

#include "../RegisteredMetaTypes.h"

enum class ColorChannelMapping {
    NONE,
    RED,
    GREEN,
    BLUE,
    ALPHA,
    COUNT
};

namespace std {
    template <>
    struct hash<ColorChannelMapping> {
        size_t operator()(const ColorChannelMapping& a) const {
            size_t result = 0;
            hash_combine(result, (int)a);
            return result;
        }
    };
};

#endif // hifi_ColorChannelMapping_h
