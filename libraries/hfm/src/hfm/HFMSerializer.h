//
//  FBXSerializer.h
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/11/07.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HFMSerializer_h
#define hifi_HFMSerializer_h

#include <shared/HifiTypes.h>

#include "HFM.h"

namespace hfm {

class Serializer {
    virtual Model::Pointer read(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url = hifi::URL()) = 0;
};

};

using HFMSerializer = hfm::Serializer;

#endif // hifi_HFMSerializer_h
