//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_StaticRayPick_h
#define hifi_StaticRayPick_h

#include "RayPick.h"

class StaticRayPick : public RayPick {

public:
    StaticRayPick(const glm::vec3& position, const glm::vec3& direction, const PickFilter& filter, float maxDistance = 0.0f, bool enabled = false);

    PickRay getMathematicalPick() const override;

private:
    PickRay _pickRay;

};

#endif // hifi_StaticRayPick_h
