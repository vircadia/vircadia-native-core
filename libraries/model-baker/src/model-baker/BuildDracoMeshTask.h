//
//  BuildDracoMeshTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/02/20.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BuildDracoMeshTask_h
#define hifi_BuildDracoMeshTask_h

#include <hfm/HFM.h>
#include <shared/HifiTypes.h>

#include "Engine.h"
#include "BakerTypes.h"

// BuildDracoMeshTask is disabled by default
class BuildDracoMeshConfig : public baker::JobConfig {
    Q_OBJECT
    Q_PROPERTY(int encodeSpeed MEMBER encodeSpeed)
    Q_PROPERTY(int decodeSpeed MEMBER decodeSpeed)
public:
    BuildDracoMeshConfig() : baker::JobConfig(false) {}

    int encodeSpeed { 0 };
    int decodeSpeed { 5 };
};

class BuildDracoMeshTask {
public:
    using Config = BuildDracoMeshConfig;
    using Input = baker::VaryingSet3<std::vector<hfm::Mesh>, baker::NormalsPerMesh, baker::TangentsPerMesh>;
    using Output = baker::VaryingSet3<std::vector<hifi::ByteArray>, std::vector<bool>, std::vector<std::vector<hifi::ByteArray>>>;
    using JobModel = baker::Job::ModelIO<BuildDracoMeshTask, Input, Output, Config>;

    void configure(const Config& config);
    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);

protected:
    int _encodeSpeed { 0 };
    int _decodeSpeed { 5 };
};

#endif // hifi_BuildDracoMeshTask_h
