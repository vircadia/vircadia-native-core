//
//  Created by Sam Gondelman 7/2/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MouseParabolaPick_h
#define hifi_MouseParabolaPick_h

#include "ParabolaPick.h"

class MouseParabolaPick : public ParabolaPick {

public:
    MouseParabolaPick(float speed, const glm::vec3& accelerationAxis, bool rotateAccelerationWithAvatar, bool scaleWithAvatar,
                      const PickFilter& filter, float maxDistance = 0.0f, bool enabled = false);

    PickParabola getMathematicalPick() const override;

    bool isMouse() const override { return true; }
};

#endif // hifi_MouseParabolaPick_h
