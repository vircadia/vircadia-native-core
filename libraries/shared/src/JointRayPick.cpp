//
//  JointRayPick.cpp
//  libraries/shared/src
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "JointRayPick.h"

JointRayPick::JointRayPick(const QString& jointName, const Transform& offsetTransform, const uint16_t filter, const float maxDistance, const bool enabled) :
    RayPick(filter, maxDistance, enabled),
    _jointName(jointName),
    _offsetTransform(offsetTransform)
{
}

const PickRay JointRayPick::getPickRay() {
    // TODO:
    // get pose for _jointName
    // apply _offsetTransform
    // create and return PickRay
    return PickRay();
}
