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

class Interpolate {

public:
    // Cubic interpolation at position u [0.0 - 1.0] between values y1 and y2 with equidistant values y0 and y3 either side.
    static float cubicInterpolate2Points(float y0, float y1, float y2, float y3, float u);

    // Cubic interpolation at position u [0.0 - 1.0] between values y1 and y3 with midpoint value y2.
    static float cubicInterpolate3Points(float y1, float y2, float y3, float u);
};

#endif // hifi_Interpolate_h
