//
//  SimpleMovingAverage.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/18/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "SharedUtil.h"
#include "SimpleMovingAverage.h"

SimpleMovingAverage::SimpleMovingAverage(int numSamplesToAverage) :
    _numSamples(0),
    _average(0),
    _eventDeltaAverage(0),
    WEIGHTING(1.0f / numSamplesToAverage),
    ONE_MINUS_WEIGHTING(1 - WEIGHTING) {
        
}

int SimpleMovingAverage::updateAverage(float sample) {
    if (_numSamples > 0) {
        _average = (ONE_MINUS_WEIGHTING * _average) + (WEIGHTING * sample);
        
        float eventDelta = (usecTimestampNow() - _lastEventTimestamp) / 1000000.0f;
        
        if (_numSamples > 1) {
            _eventDeltaAverage = (ONE_MINUS_WEIGHTING * _eventDeltaAverage) +
                (WEIGHTING * eventDelta);
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

float SimpleMovingAverage::getEventDeltaAverage() {
    return (ONE_MINUS_WEIGHTING * _eventDeltaAverage) +
        (WEIGHTING * ((usecTimestampNow() - _lastEventTimestamp) / 1000000.0f));
}

float SimpleMovingAverage::getAverageSampleValuePerSecond() {
    return _average * (1 / getEventDeltaAverage());
}