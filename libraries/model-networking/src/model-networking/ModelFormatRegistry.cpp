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

#include "FBXSerializer.h"
#include "OBJSerializer.h"
#include "GLTFSerializer.h"

ModelFormatRegistry::ModelFormatRegistry() : hfm::FormatRegistry() {
    addDefaultFormats();
}

void ModelFormatRegistry::addDefaultFormats() {
    addFormat(FBXSerializer::FORMAT);
    addFormat(OBJSerializer::FORMAT);
    addFormat(GLTFSerializer::FORMAT);
}

void ModelFormatRegistry::addFormat(const std::shared_ptr<hfm::Format>& format) {
    format->registerFormat(*this);
    withWriteLock([&](){
        formats.push_back(format);
    });
}

ModelFormatRegistry::~ModelFormatRegistry() {
    for (auto& format : formats) {
        format->unregisterFormat(*this);
    }
    formats.clear();
}
