//
//  Created by Sam Gondelman 7/2/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "StaticParabolaPick.h"

StaticParabolaPick::StaticParabolaPick(const glm::vec3& position, const glm::vec3& direction, float speed, const glm::vec3& accelerationAxis, bool rotateWithAvatar,
    const PickFilter& filter, float maxDistance, bool enabled) :
    ParabolaPick(speed, accelerationAxis, rotateWithAvatar, filter, maxDistance, enabled),
    _position(position), _velocity(speed * direction)
{
}

PickParabola StaticParabolaPick::getMathematicalPick() const {
    return PickParabola(_position, _velocity, getAcceleration());
}