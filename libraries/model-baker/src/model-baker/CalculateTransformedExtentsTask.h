//
//  CalculateTransformedExtentsTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/10/04.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_CalculateExtentsTask_h
#define hifi_CalculateExtentsTask_h

#include "Engine.h"
#include "hfm/HFM.h"

// Calculates any undefined extents in the shapes and the model. Precalculated extents will be left alone.
// Bind extents will currently not be calculated
class CalculateTransformedExtentsTask {
public:
    using Input = baker::VaryingSet4<Extents, std::vector<hfm::TriangleListMesh>, std::vector<hfm::Shape>, std::vector<hfm::Joint>>;
    using Output = baker::VaryingSet2<Extents, std::vector<hfm::Shape>>;
    using JobModel = baker::Job::ModelIO<CalculateTransformedExtentsTask, Input, Output>;

    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);
};

#endif // hifi_CalculateExtentsTask_h
