//
//  PrepareJointsTask.h
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/01/25.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PrepareJointsTask_h
#define hifi_PrepareJointsTask_h

#include <QHash>

#include <hfm/HFM.h>

#include "Engine.h"
#include "BakerTypes.h"

class PrepareJointsTask {
public:
    using Input = baker::VaryingSet2<std::vector<hfm::Joint>, baker::GeometryMappingPair /*mapping*/>;
    using Output = baker::VaryingSet3<std::vector<hfm::Joint>, QMap<int, glm::quat> /*jointRotationOffsets*/, QHash<QString, int> /*jointIndices*/>;
    using JobModel = baker::Job::ModelIO<PrepareJointsTask, Input, Output>;

    void run(const baker::BakeContextPointer& context, const Input& input, Output& output);
};

#endif // hifi_PrepareJointsTask_h