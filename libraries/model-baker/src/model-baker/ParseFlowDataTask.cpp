//
//  Created by Luis Cuenca on 5/3/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ParseFlowDataTask.h"

void ParseFlowDataTask::run(const baker::BakeContextPointer& context, const Input& mappingPair, Output& output) {
    FlowData flowData;
    static const QString FLOW_PHYSICS_FIELD = "flowPhysicsData";
    static const QString FLOW_COLLISIONS_FIELD = "flowCollisionsData";
    auto mapping = mappingPair.second;
    for (auto mappingIter = mapping.begin(); mappingIter != mapping.end(); mappingIter++) {
        if (mappingIter.key() == FLOW_PHYSICS_FIELD || mappingIter.key() == FLOW_COLLISIONS_FIELD) {
            QByteArray data = mappingIter.value().toByteArray();
            QJsonObject dataObject = QJsonDocument::fromJson(data).object();
            if (!dataObject.isEmpty() && dataObject.keys().size() == 1) {
                QString key = dataObject.keys()[0];
                if (dataObject[key].isObject()) {
                    QVariantMap dataMap = dataObject[key].toObject().toVariantMap();
                    if (mappingIter.key() == FLOW_PHYSICS_FIELD) {
                        flowData._physicsConfig.insert(key, dataMap);
                    } else {
                        flowData._collisionsConfig.insert(key, dataMap);
                    }
                }
            }
        }
    }
    output = flowData;
}
