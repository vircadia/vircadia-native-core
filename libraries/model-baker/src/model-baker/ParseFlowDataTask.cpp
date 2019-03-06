//
//  Created by Luis Cuenca on 5/3/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ParseFlowDataTask.h"

void ParseFlowDataTask::run(const baker::BakeContextPointer& context, const Input& mapping, Output& output) {
    FlowData flowData;
    static const QString FLOW_PHYSICS_FIELD = "flowPhysicsData";
    static const QString FLOW_COLLISIONS_FIELD = "flowCollisionsData";
    for (auto &mappingIter = mapping.begin(); mappingIter != mapping.end(); mappingIter++) {
        if (mappingIter.key() == FLOW_PHYSICS_FIELD || mappingIter.key() == FLOW_COLLISIONS_FIELD) {
            QByteArray flowDataValue = mappingIter.value().toByteArray();
            if (mappingIter.key() == FLOW_PHYSICS_FIELD) {
                flowData._physicsData.push_back(flowDataValue);
            } else {
                flowData._collisionsData.push_back(flowDataValue);
            }
        }
    }
    output = flowData;
}
