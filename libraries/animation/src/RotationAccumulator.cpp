//
//  RotationAccumulator.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RotationAccumulator.h"

#include <glm/gtx/quaternion.hpp>

glm::quat RotationAccumulator::getAverage() {
    glm::quat average;
    uint32_t numRotations = _rotations.size();
    if (numRotations > 0) {
        average = _rotations[0];
        for (uint32_t i = 1; i < numRotations; ++i) {
            glm::quat rotation = _rotations[i];
            if (glm::dot(average, rotation) < 0.0f) {
                rotation = -rotation;
            }
            average += rotation;
        }
        average = glm::normalize(average);
    }
    return average;
}
