//
//  FaceTracker.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 4/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FaceTracker.h"

inline float FaceTracker::getBlendshapeCoefficient(int index) const {
    return isValidBlendshapeIndex(index) ? _blendshapeCoefficients[index] : 0.0f;
}
