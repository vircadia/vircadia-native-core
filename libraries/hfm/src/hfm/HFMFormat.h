//
//  HFMFormat.h
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/11/30.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFMFormat_h
#define hifi_HFMFormat_h

#include "HFMFormatRegistry.h"

namespace hfm {
    class Format {
    public:
        virtual void registerFormat(FormatRegistry& registry) = 0;
        virtual void unregisterFormat(FormatRegistry& registry) = 0;
    };
};

#endif // hifi_HFMFormat_h
