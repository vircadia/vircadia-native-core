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

StDev::StDev() :
    _data(),
    _sampleCount(0)
{
    reset();
}

void StDev::reset() {
    memset(&_data, 0, sizeof(_data));
    _sampleCount = 0;
}

void StDev::addValue(float v) {
    _data[_sampleCount++] = v;
    if (_sampleCount == NUM_SAMPLES) _sampleCount = 0;
}

float StDev::getAverage() const {
    float average = 0;
    for (int i = 0; i < _sampleCount; i++) {
        average += _data[i];
    }
    if (_sampleCount > 0)
        return average / (float)_sampleCount;
    else return 0;
}

float StDev::getStDev() const {
    float average = getAverage();
    float stdev = 0;
    for (int i = 0; i < _sampleCount; i++) {
        stdev += powf(_data[i] - average, 2);
    }
    if (_sampleCount > 1)
        return sqrt(stdev / (float)(_sampleCount - 1.0f));
    else
        return 0;
}
