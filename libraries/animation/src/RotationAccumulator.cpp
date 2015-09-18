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

void RotationAccumulator::add(glm::quat rotation) {
    if (_numRotations == 0) {
        _rotationSum = rotation;
    } else {
        if (glm::dot(_rotationSum, rotation) < 0.0f) {
            rotation = -rotation;
        }
        _rotationSum += rotation;
    }
    ++_numRotations;
}

glm::quat RotationAccumulator::getAverage() {
    return (_numRotations > 0) ?  glm::normalize(_rotationSum) : glm::quat();
}
