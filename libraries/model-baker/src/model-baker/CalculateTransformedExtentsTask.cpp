//
//  CalculateTransformedExtentsTask.cpp
//  model-baker/src/model-baker
//
//  Created by Sabrina Shanman on 2019/10/04.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CalculateTransformedExtentsTask.h"

#include "hfm/HFMModelMath.h"

void CalculateTransformedExtentsTask::run(const baker::BakeContextPointer& context, const Input& input, Output& output) {
    const auto& modelExtentsIn = input.get0();
    const auto& triangleListMeshes = input.get1();
    const auto& shapesIn = input.get2();
    const auto& joints = input.get3();
    auto& modelExtentsOut = output.edit0();
    auto& shapesOut = output.edit1();

    shapesOut.reserve(shapesIn.size());
    for (size_t i = 0; i < shapesIn.size(); ++i) {
        shapesOut.push_back(shapesIn[i]);
        auto& shapeOut = shapesOut.back();

        auto& shapeExtents = shapeOut.transformedExtents;
        if (shapeExtents.isValid()) {
            continue;
        }

        hfm::calculateExtentsForShape(shapeOut, triangleListMeshes, joints);
    }

    modelExtentsOut = modelExtentsIn;
    if (!modelExtentsOut.isValid()) {
        hfm::calculateExtentsForModel(modelExtentsOut, shapesOut);
    }
}
