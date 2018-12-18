//
//  ModelLoader.h
//  libraries/model-networking/src/model-networking
//
//  Created by Sabrina Shanman on 2018/11/13.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelLoader_h
#define hifi_ModelLoader_h

#include <shared/HifiTypes.h>
#include <hfm/HFM.h>

class ModelLoader {
public:
    // Given the currently stored list of supported file formats, determine how to load a model from the given parameters.
    // If successful, return an owned reference to the newly loaded model.
    // If failed, return an empty reference.
    hfm::Model::Pointer load(const hifi::ByteArray& data, const hifi::VariantHash& mapping, const hifi::URL& url, const std::string& webMediaType) const;
};

#endif // hifi_ModelLoader_h
