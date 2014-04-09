//
//  InputVertex.cpp
//  interface/src/starfield/data
//
//  Created by Chris Barnard on 10/17/13.
//  Based on code by Tobias Schwinger 3/29/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "starfield/data/InputVertex.h"

using namespace starfield;

InputVertex::InputVertex(float azimuth, float altitude, unsigned color) {
    _color = color | 0xff000000u;
    
    azimuth = angleConvert<Degrees,Radians>(azimuth);
    altitude = angleConvert<Degrees,Radians>(altitude);
    
    angleHorizontalPolar<Radians>(azimuth, altitude);

    _azimuth = azimuth;
    _altitude = altitude;
}
