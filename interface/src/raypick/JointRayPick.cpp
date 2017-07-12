//
//  JointRayPick.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "JointRayPick.h"

JointRayPick::JointRayPick(const QString& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const uint16_t filter, const float maxDistance, const bool enabled) :
    RayPick(filter, maxDistance, enabled),
    _jointName(jointName),
    _posOffset(posOffset),
    _dirOffset(dirOffset)
{
}

const PickRay JointRayPick::getPickRay() {
    // TODO:
    // get pose for _jointName
    // apply offset
    // create and return PickRay
    return PickRay();
}
