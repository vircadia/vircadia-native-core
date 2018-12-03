//
//  ModelFormatRegistry.h
//  libraries/model-networking/src/model-networking
//
//  Created by Sabrina Shanman on 2018/11/30.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelFormatRegistry_h
#define hifi_ModelFormatRegistry_h

#include <hfm/HFMFormat.h>
#include <hfm/HFMFormatRegistry.h>

#include <DependencyManager.h>

class ModelFormatRegistry : public hfm::FormatRegistry, Dependency {
public:
    ModelFormatRegistry();
    ~ModelFormatRegistry();
    void addFormat(const std::shared_ptr<hfm::Format>& format);

protected:
    void addDefaultFormats();
    std::vector<std::shared_ptr<hfm::Format>> formats;
    std::mutex _formatsLock;
};

#endif // hifi_ModelFormatRegistry_h
