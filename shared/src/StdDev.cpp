//
//  StdDev.cpp
//  hifi
//
//  Created by Philip Rosedale on 3/12/13.
//
//

#include "StdDev.h"
#include <cmath>

const int MAX_STDEV_SAMPLES = 1000;                     //  Don't add more than this number of samples.

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

float StDev::getAverage() {
    float average = 0;
    for (int i = 0; i < sampleCount; i++) {
        average += data[i];
    }
    if (sampleCount > 0)
        return average/(float)sampleCount;
    else return 0;
}

float StDev::getStDev() {
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