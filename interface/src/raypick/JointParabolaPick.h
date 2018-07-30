//
//  Created by Sam Gondelman 7/2/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_JointParabolaPick_h
#define hifi_JointParabolaPick_h

#include "ParabolaPick.h"

class JointParabolaPick : public ParabolaPick {

public:
    JointParabolaPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset,
        float speed, const glm::vec3& accelerationAxis, bool rotateAccelerationWithAvatar, bool scaleWithAvatar,
        PickFilter& filter, float maxDistance = 0.0f, bool enabled = false);

    PickParabola getMathematicalPick() const override;

    bool isLeftHand() const override { return (_jointName == "_CONTROLLER_LEFTHAND") || (_jointName == "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND"); }
    bool isRightHand() const override { return (_jointName == "_CONTROLLER_RIGHTHAND") || (_jointName == "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND"); }

private:
    std::string _jointName;
    glm::vec3 _posOffset;
    glm::vec3 _dirOffset;

};

#endif // hifi_JointParabolaPick_h
