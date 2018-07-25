//
//  Created by Sam Gondelman 7/2/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_StaticParabolaPick_h
#define hifi_StaticParabolaPick_h

#include "ParabolaPick.h"

class StaticParabolaPick : public ParabolaPick {

public:
    StaticParabolaPick(const glm::vec3& position, const glm::vec3& direction, float speed, const glm::vec3& accelerationAxis, bool rotateAccelerationWithAvatar,
                       bool scaleWithAvatar, const PickFilter& filter, float maxDistance = 0.0f, bool enabled = false);

    PickParabola getMathematicalPick() const override;

private:
    glm::vec3 _position;
    glm::vec3 _velocity;
};

#endif // hifi_StaticParabolaPick_h
