//
//  ReweightDeformersTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/09/26.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ReweightDeformersTask_h
#define hifi_ReweightDeformersTask_h

#include <hfm/HFM.h>

#include "Engine.h"
#include "BakerTypes.h"

class ReweightDeformersTask {
public:
    using Input = baker::VaryingSet4<std::vector<hfm::Mesh>, std::vector<hfm::Shape>, std::vector<hfm::DynamicTransform>, std::vector<hfm::Deformer>>;
    using Output = std::vector<baker::ReweightedDeformers>;
    using JobModel = baker::Job::ModelIO<ReweightDeformersTask, Input, Output>;

    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);
};

#endif // hifi_ReweightDeformersTask_h
