//
//  GpuVertex.cpp
//  interface/src/starfield/data
//
//  Created by Chris Barnard on 10/17/13.
//  Based on code by Tobias Schwinger on 3/29/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "starfield/data/GpuVertex.h"

using namespace starfield;

GpuVertex::GpuVertex(InputVertex const& inputVertex) {
    _color = inputVertex.getColor();
    float azimuth = inputVertex.getAzimuth();
    float altitude = inputVertex.getAltitude();
    
    // compute altitude/azimuth into X/Y/Z point on a sphere
    _valX = sin(azimuth) * cos(altitude);
    _valY = sin(altitude);
    _valZ = -cos(azimuth) * cos(altitude);
}
