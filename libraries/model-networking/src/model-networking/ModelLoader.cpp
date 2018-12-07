//
//  ModelLoader.cpp
//  libraries/model-networking/src/model-networking
//
//  Created by Sabrina Shanman on 2018/11/14.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelLoader.h"

#include <DependencyManager.h>
#include <hfm/ModelFormatRegistry.h>


hfm::Model::Pointer ModelLoader::load(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url, const std::string& webMediaType) const {
    auto serializer = DependencyManager::get<ModelFormatRegistry>()->getSerializerForMediaType(data, url, webMediaType);
    if (!serializer) {
        return hfm::Model::Pointer();
    }
    return serializer->read(data, mapping, url);
}
