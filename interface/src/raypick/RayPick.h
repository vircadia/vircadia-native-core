//
//  RayPick.h
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_RayPick_h
#define hifi_RayPick_h

#include <stdint.h>
#include "RegisteredMetaTypes.h"

class RayPick {

public:
	RayPick(const uint16_t filter, const float maxDistance, const bool enabled);

    virtual const PickRay getPickRay() = 0;

    void enable() { _enabled = true; }
    void disable() { _enabled = false; }

    const uint16_t getFilter() { return _filter; }
    const float getMaxDistance() { return _maxDistance; }
    const bool isEnabled() { return _enabled; }
    //const IntersectionResult& getLastIntersectionResult() { return _prevIntersectionResult; }

    //void setIntersectionResult(const IntersectionResult& intersectionResult) { _prevIntersectionResult = intersectionResult; }

private:
    uint16_t _filter;
    float _maxDistance;
    bool _enabled;
    //IntersectionResult _prevIntersectionResult; // set to invalid on disable()?

};

#endif hifi_RayPick_h