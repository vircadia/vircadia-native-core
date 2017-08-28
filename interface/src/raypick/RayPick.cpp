//
//  RayPick.cpp
//  interface/src/raypick
//
//  Created by Sam Gondelman 7/11/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RayPick.h"

RayPick::RayPick(const RayPickFilter& filter, const float maxDistance, const bool enabled) :
    _filter(filter),
    _maxDistance(maxDistance),
    _enabled(enabled)
{
}
