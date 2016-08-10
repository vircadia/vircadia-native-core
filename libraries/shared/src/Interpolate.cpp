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

#include <assert.h>
#include <math.h>

#include "NumericalConstants.h"

float Interpolate::bezierInterpolate(float y1, float y2, float y3, float u) {
    // https://en.wikipedia.org/wiki/Bezier_curve
    assert(0.0f <= u && u <= 1.0f);
    return (1.0f - u) * (1.0f - u) * y1 + 2.0f * (1.0f - u) * u * y2 + u * u * y3;
}

float Interpolate::interpolate3Points(float y1, float y2, float y3, float u) {
    assert(0.0f <= u && u <= 1.0f);

    if ((u <= 0.5f && y1 == y2) || (u >= 0.5f && y2 == y3)) {
        // Flat line.
        return y2;
    }

    if ((y2 >= y1 && y2 >= y3) || (y2 <= y1 && y2 <= y3)) {
        // U or inverted-U shape.
        // Make the slope at y2 = 0, which means that the control points half way between the value points have the value y2.
        if (u <= 0.5f) {
            return bezierInterpolate(y1, y2, y2, 2.0f * u);
        } else {
            return bezierInterpolate(y2, y2, y3, 2.0f * u - 1.0f);
        }

    } else {
        // L or inverted and/or mirrored L shape.
        // Make the slope at y2 be the slope between y1 and y3, up to a maximum of double the minimum of the slopes between y1
        // and y2, and y2 and y3. Use this slope to calculate the control points half way between the value points.
        // Note: The maximum ensures that the control points and therefore the interpolated values stay between y1 and y3.
        float slope = y3 - y1;
        float slope12 = y2 - y1;
        float slope23 = y3 - y2;
        if (fabsf(slope) > fabsf(2.0f * slope12)) {
            slope = 2.0f * slope12;
        } else if (fabsf(slope) > fabsf(2.0f * slope23)) {
            slope = 2.0f * slope23;
        }

        if (u <= 0.5f) {
            return bezierInterpolate(y1, y2 - slope / 2.0f, y2, 2.0f * u);
        } else {
            return bezierInterpolate(y2, y2 + slope / 2.0f, y3, 2.0f * u - 1.0f);
        }
    }
}

float Interpolate::calculateFadeRatio(quint64 start) {
    const float FADE_TIME = 1.0f;
    float t = 2.0f * std::min(((float)(usecTimestampNow() - start)) / ((float)(FADE_TIME * USECS_PER_SECOND)), 1.0f);
    float fadeRatio = (t < 1.0f) ? 0.5f * powf(2.0f, 10.0f * (t - 1.0f)) : 0.5f * (-powf(2.0f, -10.0f * (t - 1.0f)) + 2.0f);

    // The easing function isn't exactly 1 at t = 2, so we need to scale the whole function up slightly
    const float EASING_SCALE = 1.001f;
    return std::min(EASING_SCALE * fadeRatio, 1.0f);
}