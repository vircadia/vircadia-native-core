//
//  ModelFormatRegistry.h
//  libraries/hfm/src/hfm
//
//  Created by Sabrina Shanman on 2018/11/30.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelFormatRegistry_h
#define hifi_ModelFormatRegistry_h

#include "HFMFormatRegistry.h"

#include <DependencyManager.h>

class ModelFormatRegistry : public Dependency {
public:
    void addFormat(const hfm::Serializer& serializer);
    std::shared_ptr<hfm::Serializer> getSerializerForMediaType(const hifi::ByteArray& data, const hifi::URL& url, const std::string& webMediaType) const;

protected:
    hfm::FormatRegistry _hfmFormatRegistry;
};

#endif // hifi_ModelFormatRegistry_h
