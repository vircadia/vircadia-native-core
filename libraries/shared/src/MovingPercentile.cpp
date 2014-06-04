#include "MovingPercentile.h"
//#include "stdio.h"// DEBUG

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

//printf("\nnew sample: %2.2f  ", sample);

    // find index in _samplesSorted to insert new sample.
    // if samples have not been filled yet, this will be the next empty spot
    // otherwise, it will be the spot of the oldest sample
    int newSampleIndex;
    if (_numExistingSamples < _numSamples) {

        newSampleIndex = _numExistingSamples;
        _numExistingSamples++;

        // update _indexOfPercentile
        float index = _percentile * (float)(_numExistingSamples - 1);
        _indexOfPercentile = (int)(index + 0.5f);   // round to int
    }
    else {
        for (int i = 0; i < _numExistingSamples; i++) {
            if (_sampleAges[i] == _numExistingSamples - 1) {
                newSampleIndex = i;
                break;
            }
        }
    }

//printf("will be inserted at index %d\n", newSampleIndex);

    // update _sampleAges to reflect new sample (age all samples by 1)
    for (int i = 0; i < _numExistingSamples; i++) {
        _sampleAges[i]++;
    }

    // insert new sample at that index
    _samplesSorted[newSampleIndex] = sample;
    _sampleAges[newSampleIndex] = 0;

    // swap new sample with neighboring elements in _samplesSorted until it's in sorted order
    // try swapping up first, then down.  element will only be swapped one direction.

    float neighborSample;
    while (newSampleIndex < _numExistingSamples-1 && sample > (neighborSample = _samplesSorted[newSampleIndex+1])) {
//printf("\t swapping up...\n");
        _samplesSorted[newSampleIndex] = neighborSample;
        _samplesSorted[newSampleIndex+1] = sample;

        _sampleAges[newSampleIndex] = _sampleAges[newSampleIndex+1];
        _sampleAges[newSampleIndex+1] = 0;

        newSampleIndex++;
    }
    while (newSampleIndex > 0 && sample < (neighborSample = _samplesSorted[newSampleIndex - 1])) {
//printf("\t swapping down...\n");
        _samplesSorted[newSampleIndex] = neighborSample;
        _samplesSorted[newSampleIndex - 1] = sample;

        _sampleAges[newSampleIndex] = _sampleAges[newSampleIndex - 1];
        _sampleAges[newSampleIndex - 1] = 0;

        newSampleIndex--;
    }

    // find new value at percentile
    _valueAtPercentile = _samplesSorted[_indexOfPercentile];
/*
printf(" new median: %f\n", _median);

// debug:
for (int i = 0; i < _numExistingSamples; i++) {
    printf("%2.2f (%d), ", _samplesSorted[i], _sampleAges[i]);
}
printf("\n\n");
*/
}