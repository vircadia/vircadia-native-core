//
//  JointRayPick.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_JointRayPick_h
#define hifi_JointRayPick_h

#include "RayPick.h"

class JointRayPick : public RayPick {

public:
    JointRayPick(const std::string& jointName, const glm::vec3& posOffset, const glm::vec3& dirOffset, const PickFilter& filter, float maxDistance = 0.0f, bool enabled = false);

    PickRay getMathematicalPick() const override;

    bool isLeftHand() const override { return (_jointName == "_CONTROLLER_LEFTHAND") || (_jointName == "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND"); }
    bool isRightHand() const override { return (_jointName == "_CONTROLLER_RIGHTHAND") || (_jointName == "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND"); }

private:
    std::string _jointName;
    glm::vec3 _posOffset;
    glm::vec3 _dirOffset;

};

#endif // hifi_JointRayPick_h
