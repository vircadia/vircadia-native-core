//
//  StdDev.cpp
//  libraries/shared/src
//
//  Created by Philip Rosedale on 3/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <limits>   
#include <cmath>
#include "StdDev.h"

const int MAX_STDEV_SAMPLES = 1000;

StDev::StDev() {
    data = new float[MAX_STDEV_SAMPLES];
    sampleCount = 0;
}

void StDev::reset() {
    sampleCount = 0;
}

void StDev::addValue(float v) {
    data[sampleCount++] = v;
    if (sampleCount == MAX_STDEV_SAMPLES) sampleCount = 0;
}

float StDev::getAverage() const {
    float average = 0;
    for (int i = 0; i < sampleCount; i++) {
        average += data[i];
    }
    if (sampleCount > 0)
        return average/(float)sampleCount;
    else return 0;
}
/*
float StDev::getMax() { 
    float average = -FLT_MAX;
    for (int i = 0; i < sampleCount; i++) {
        average += data[i];
    }
    if (sampleCount > 0)
        return average/(float)sampleCount;
    else return 0;
}*/

float StDev::getStDev() const {
    float average = getAverage();
    float stdev = 0;
    for (int i = 0; i < sampleCount; i++) {
        stdev += powf(data[i] - average, 2);
    }
    if (sampleCount > 1)
        return sqrt(stdev/(float)(sampleCount - 1.0));
    else
        return 0;
}
