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
    _samplesSorted(),
    _sampleIds(),
    _newSampleId(0),
    _indexOfPercentile(0),
    _valueAtPercentile(0)
{
}

void MovingPercentile::updatePercentile(qint64 sample) {

    // insert the new sample into _samplesSorted
    int newSampleIndex;
    if (_samplesSorted.size() < _numSamples) {
        // if not all samples have been filled yet, simply append it
        newSampleIndex = _samplesSorted.size();
        _samplesSorted.append(sample);
        _sampleIds.append(_newSampleId);

        // update _indexOfPercentile
        float index = _percentile * (float)(_samplesSorted.size() - 1);
        _indexOfPercentile = (int)(index + 0.5f);   // round to int
    } else {
        // find index of sample with id = _newSampleId and replace it with new sample
        newSampleIndex = _sampleIds.indexOf(_newSampleId);
        _samplesSorted[newSampleIndex] = sample;
    }

    // increment _newSampleId.  cycles from 0 thru N-1
    _newSampleId = (_newSampleId == _numSamples - 1) ? 0 : _newSampleId + 1;

    // swap new sample with neighbors in _samplesSorted until it's in sorted order
    // try swapping up first, then down.  element will only be swapped one direction.
    while (newSampleIndex < _samplesSorted.size() - 1 && sample > _samplesSorted[newSampleIndex + 1]) {
        _samplesSorted.swap(newSampleIndex, newSampleIndex + 1);
        _sampleIds.swap(newSampleIndex, newSampleIndex + 1);
        newSampleIndex++;
    }
    while (newSampleIndex > 0 && sample < _samplesSorted[newSampleIndex - 1]) {
        _samplesSorted.swap(newSampleIndex, newSampleIndex - 1);
        _sampleIds.swap(newSampleIndex, newSampleIndex - 1);
        newSampleIndex--;
    }

    // find new value at percentile
    _valueAtPercentile = _samplesSorted[_indexOfPercentile];
}
