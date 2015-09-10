//
//  Interpolate.cpp
//  libraries/shared/src
//
//  Created by David Rowe on 10 Sep 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Interpolate.h"

float Interpolate::cubicInterpolate2Points(float y0, float y1, float y2, float y3, float u) {
    float a0, a1, a2, a3, uu, uuu;

    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;

    uu = u * u;
    uuu = uu * u;

    return (a0 * uuu + a1 * uu + a2 * u + a3);
}

float Interpolate::cubicInterpolate3Points(float y1, float y2, float y3, float u) {
    float y0, y4;

    if (u <= 0.5f) {
        if (y1 == y2) {
            return y2;
        }

        y0 = 2.0f * y1 - y2;  // y0 is linear extension of line from y2 to y1.
        u = 2.0f * u;

        return Interpolate::cubicInterpolate2Points(y0, y1, y2, y3, u);

    } else {
        if (y2 == y3) {
            return y2;
        }

        y4 = 2.0f * y3 - y2;  // y4 is linear extension of line from y2 to y3.
        u = 2.0f * u - 1.0f;

        return Interpolate::cubicInterpolate2Points(y1, y2, y3, y4, u);
    }
}
