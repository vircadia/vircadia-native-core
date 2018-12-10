//
//  ModelFormatRegistry.cpp
//  libraries/model-networking/src/model-networking
//
//  Created by Sabrina Shanman on 2018/11/30.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelFormatRegistry.h"

void ModelFormatRegistry::addFormat(const hfm::Serializer& serializer) {
    _hfmFormatRegistry.registerMediaType(serializer.getMediaType(), serializer.getFactory());
}

std::shared_ptr<hfm::Serializer> ModelFormatRegistry::getSerializerForMediaType(const hifi::ByteArray& data, const hifi::URL& url, const std::string& webMediaType) const {
    return _hfmFormatRegistry.getSerializerForMediaType(data, url, webMediaType);
}
