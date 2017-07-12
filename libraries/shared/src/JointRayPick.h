//
//  JointRayPick.h
//  libraries/shared/src
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

#include <QString>
#include "Transform.h"

class JointRayPick : public RayPick {

public:
    JointRayPick(const QString& jointName, const Transform& offsetTransform, const uint16_t filter, const float maxDistance = 0.0f, const bool enabled = false);

    const PickRay getPickRay() override;

private:
    QString _jointName;
    Transform _offsetTransform;

};

#endif hifi_JointRayPick_h