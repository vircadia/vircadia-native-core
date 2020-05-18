//
//  CollectShapeVerticesTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/09/27.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CollectShapeVerticesTask_h
#define hifi_CollectShapeVerticesTask_h

#include <hfm/HFM.h>

#include "Engine.h"
#include "BakerTypes.h"

class CollectShapeVerticesTask {
public:
    using Input = baker::VaryingSet4<std::vector<hfm::Mesh>, std::vector<hfm::Shape>, std::vector<hfm::Joint>, std::vector<hfm::SkinDeformer>>;
    using Output = std::vector<ShapeVertices>;
    using JobModel = baker::Job::ModelIO<CollectShapeVerticesTask, Input, Output>;

    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);
};

#endif // hifi_CollectShapeVerticesTask_h

