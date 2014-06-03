#include "MovingMedian.h"
//#include "stdio.h"// DEBUG

MovingMedian::MovingMedian(int numSamples)
    : _numSamples(numSamples),
    _numExistingSamples(0),
    _median(0.0f)
{
    _samplesSorted = new float[numSamples];
    _sampleAges = new int[numSamples];
}

MovingMedian::~MovingMedian() {
    delete[] _samplesSorted;
    delete[] _sampleAges;
}

void MovingMedian::updateMedian(float sample) {

//printf("\nnew sample: %2.2f  ", sample);

    // find index in _samplesSorted to insert new sample.
    // if samples have not been filled yet, this will be the next empty spot
    // otherwise, it will be the spot of the oldest sample
    int newSampleIndex;
    if (_numExistingSamples < _numSamples) {
        newSampleIndex = _numExistingSamples;
        _numExistingSamples++;
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


    // find new median
    _median = _samplesSorted[_numExistingSamples/2];
/*
printf(" new median: %f\n", _median);

// debug:
for (int i = 0; i < _numExistingSamples; i++) {
    printf("%2.2f (%d), ", _samplesSorted[i], _sampleAges[i]);
}
printf("\n\n");
*/
}