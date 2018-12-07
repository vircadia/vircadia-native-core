//
//  FormatSerializerRegister.cpp
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/12/07.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FormatSerializerRegister.h"

#include "ModelFormatRegistry.h"

namespace hfm {

DoFormatSerializerRegister::DoFormatSerializerRegister(const MediaType& mediaType, std::unique_ptr<Serializer::Factory> factory) {
    auto registry = DependencyManager::get<ModelFormatRegistry>();
    _mediaTypeID = registry->_hfmFormatRegistry.registerMediaType(mediaType, std::move(factory));
}

void DoFormatSerializerRegister::unregisterFormat() {
    auto registry = DependencyManager::get<ModelFormatRegistry>();
    registry->_hfmFormatRegistry.unregisterMediaType(_mediaTypeID);
    _mediaTypeID = hfm::FormatRegistry::INVALID_MEDIA_TYPE_ID;
}

};
