//
//  MouseRayPick.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/19/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_MouseRayPick_h
#define hifi_MouseRayPick_h

#include "RayPick.h"

class MouseRayPick : public RayPick {

public:
    MouseRayPick(const PickFilter& filter, float maxDistance = 0.0f, bool enabled = false);

    PickRay getMathematicalPick() const override;

    bool isMouse() const override { return true; }
};

#endif // hifi_MouseRayPick_h
