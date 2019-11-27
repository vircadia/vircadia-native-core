//
//  Baker.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2018/12/04.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_baker_Baker_h
#define hifi_baker_Baker_h

#include <shared/HifiTypes.h>
#include <hfm/HFM.h>

#include "Engine.h"
#include "BakerTypes.h"

#include "ParseMaterialMappingTask.h"

namespace baker {
    class Baker {
    public:
        Baker(const hfm::Model::Pointer& hfmModel, const hifi::VariantHash& mapping, const hifi::URL& materialMappingBaseURL);

        std::shared_ptr<TaskConfig> getConfiguration();

        void run();

        // Outputs, available after run() is called
        hfm::Model::Pointer getHFMModel() const;
        MaterialMapping getMaterialMapping() const;
        const std::vector<hifi::ByteArray>& getDracoMeshes() const;
        std::vector<bool> getDracoErrors() const;
        // This is a ByteArray and not a std::string because the character sequence can contain the null character (particularly for FBX materials)
        std::vector<std::vector<hifi::ByteArray>> getDracoMaterialLists() const;

    protected:
        EnginePointer _engine;
    };
};

#endif //hifi_baker_Baker_h
