//
//  BuildGraphicsMeshTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2018/12/06.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BuildGraphicsMeshTask_h
#define hifi_BuildGraphicsMeshTask_h

#include <hfm/HFM.h>
#include <shared/HifiTypes.h>

#include "Engine.h"
#include "BakerTypes.h"

class BuildGraphicsMeshTask {
public:
    using Input = baker::VaryingSet5<std::vector<hfm::Mesh>, hifi::URL, baker::MeshIndicesToModelNames, baker::NormalsPerMesh, baker::TangentsPerMesh>;
    using Output = std::vector<graphics::MeshPointer>;
    using JobModel = baker::Job::ModelIO<BuildGraphicsMeshTask, Input, Output>;

    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);
};

#endif // hifi_BuildGraphicsMeshTask_h
