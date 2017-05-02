//
//  Interpolate.h
//  libraries/shared/src
//
//  Created by David Rowe on 10 Sep 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Interpolate_h
#define hifi_Interpolate_h

#include "SharedUtil.h"

class Interpolate {

public:
    // Bezier interpolate at position u [0.0 - 1.0] between y values equally spaced along the x-axis. The interpolated values
    // pass through y1 and y3 but not y2; y2 is the Bezier control point.
    static float bezierInterpolate(float y1, float y2, float y3, float u);

    // Interpolate at position u [0.0 - 1.0] between y values equally spaced along the x-axis such that the interpolated values
    // pass through all three y values. Return value lies wholly within the range of y values passed in.
    static float interpolate3Points(float y1, float y2, float y3, float u);

    // returns smooth in and out blend between 0 and 1
    // DANGER: assumes fraction is properly inside range [0, 1]
    static float simpleNonLinearBlend(float fraction);

    static float calculateFadeRatio(quint64 start);

    // Basic ease-in-ease-out function for smoothing values.
    static float easeInOutQuad(float lerpValue);
};

#endif // hifi_Interpolate_h
