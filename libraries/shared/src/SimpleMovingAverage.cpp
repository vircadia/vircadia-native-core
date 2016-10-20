//
//  SimpleMovingAverage.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 4/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SharedUtil.h"
#include "SimpleMovingAverage.h"

SimpleMovingAverage::SimpleMovingAverage(int numSamplesToAverage) :
    _numSamples(0),
    _lastEventTimestamp(0),
    _average(0.0f),
    _eventDeltaAverage(0.0f),
    WEIGHTING(1.0f / numSamplesToAverage),
    ONE_MINUS_WEIGHTING(1 - WEIGHTING) {
}

SimpleMovingAverage::SimpleMovingAverage(const SimpleMovingAverage& other) {
    *this = other;
}

SimpleMovingAverage& SimpleMovingAverage::operator=(const SimpleMovingAverage& other) {
    _numSamples = (int)other._numSamples;
    _lastEventTimestamp = (uint64_t)other._lastEventTimestamp;
    _average = (float)other._average;
    _eventDeltaAverage = (float)other._eventDeltaAverage;
    WEIGHTING = other.WEIGHTING;
    ONE_MINUS_WEIGHTING = other.ONE_MINUS_WEIGHTING;
    return *this;
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
    _average = 0.0f;
    _eventDeltaAverage = 0.0f;
}

float SimpleMovingAverage::getEventDeltaAverage() const {
    return (ONE_MINUS_WEIGHTING * _eventDeltaAverage) +
        (WEIGHTING * ((usecTimestampNow() - _lastEventTimestamp) / 1000000.0f));
}

uint64_t SimpleMovingAverage::getUsecsSinceLastEvent() const {
    return usecTimestampNow() - _lastEventTimestamp; 
}
