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
    MouseRayPick(const RayPickFilter& filter, const float maxDistance = 0.0f, const bool enabled = false);

    const PickRay getPickRay(bool& valid) const override;
};

#endif // hifi_MouseRayPick_h
