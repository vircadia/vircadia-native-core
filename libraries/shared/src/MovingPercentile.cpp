//
//  MovingPercentile.cpp
//  libraries/shared/src
//
//  Created by Yixin Wang on 6/4/2014
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MovingPercentile.h"

MovingPercentile::MovingPercentile(int numSamples, float percentile)
    : _numSamples(numSamples),
    _percentile(percentile),
    _numExistingSamples(0),
    _valueAtPercentile(0.0f),
    _indexOfPercentile(0)
{
    _samplesSorted = new float[numSamples];
    _sampleAges = new int[numSamples];
}

MovingPercentile::~MovingPercentile() {
    delete[] _samplesSorted;
    delete[] _sampleAges;
}

void MovingPercentile::updatePercentile(float sample) {

    // age all current samples by 1
    for (int i = 0; i < _numExistingSamples; i++) {
        _sampleAges[i]++;
    }

    // find index at which to insert new sample in _samplesSorted
    int newSampleIndex;
    if (_numExistingSamples < _numSamples) {
        // if samples have not been filled yet, this will be the next empty spot
        newSampleIndex = _numExistingSamples;
        _numExistingSamples++;

        // update _indexOfPercentile
        float index = _percentile * (float)(_numExistingSamples - 1);
        _indexOfPercentile = (int)(index + 0.5f);   // round to int
    }
    else {
        // if samples have been filled, it will be the spot of the oldest sample
        newSampleIndex = 0;
        while (_sampleAges[newSampleIndex] != _numExistingSamples) { newSampleIndex++; }
    }

    // insert new sample
    _samplesSorted[newSampleIndex] = sample;
    _sampleAges[newSampleIndex] = 0;

    // swap new sample with neighboring elements in _samplesSorted until it's in sorted order
    // try swapping up first, then down.  element will only be swapped one direction.
    while (newSampleIndex < _numExistingSamples-1 && sample > _samplesSorted[newSampleIndex+1]) {
        _samplesSorted[newSampleIndex] = _samplesSorted[newSampleIndex + 1];
        _samplesSorted[newSampleIndex+1] = sample;

        _sampleAges[newSampleIndex] = _sampleAges[newSampleIndex+1];
        _sampleAges[newSampleIndex+1] = 0;

        newSampleIndex++;
    }
    while (newSampleIndex > 0 && sample < _samplesSorted[newSampleIndex - 1]) {
        _samplesSorted[newSampleIndex] = _samplesSorted[newSampleIndex - 1];
        _samplesSorted[newSampleIndex - 1] = sample;

        _sampleAges[newSampleIndex] = _sampleAges[newSampleIndex - 1];
        _sampleAges[newSampleIndex - 1] = 0;

        newSampleIndex--;
    }

    // find new value at percentile
    _valueAtPercentile = _samplesSorted[_indexOfPercentile];
}
