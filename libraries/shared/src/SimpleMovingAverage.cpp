//
//  SimpleMovingAverage.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/18/13.
//
//

#include "SharedUtil.h"
#include "SimpleMovingAverage.h"

SimpleMovingAverage::SimpleMovingAverage(float numSamplesToAverage) :
    _numSamples(0),
    _numSamplesToAverage(numSamplesToAverage),
    _average(0),
    _eventDeltaAverage(0) {
}

int SimpleMovingAverage::updateAverage(float sample) {
    if (_numSamples > 0) {
        
        float firstCoefficient = 1 - (1.0f / _numSamplesToAverage);
        float secondCoefficient = (1.0f / _numSamplesToAverage);
        
        _average = (firstCoefficient * _average) + (secondCoefficient * sample);
        
        float eventDelta = (usecTimestampNow() - _lastEventTimestamp) / 1000000;
        
        if (_numSamples > 1) {
            _eventDeltaAverage = (firstCoefficient * _eventDeltaAverage) +
                                      (secondCoefficient * eventDelta);
        } else {
            _eventDeltaAverage = eventDelta;
        }
    } else {
        _average = sample;
        _eventDeltaAverage = 0;
    }
    
    _lastEventTimestamp =  usecTimestampNow();
    
    return ++_numSamples;
}

void SimpleMovingAverage::reset() {
    _numSamples = 0;
}

float SimpleMovingAverage::getAverageSampleValuePerSecond() {
    return _average * (1 / _eventDeltaAverage);
}