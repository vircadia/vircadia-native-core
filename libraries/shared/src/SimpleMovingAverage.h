//
//  SimpleMovingAverage.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 4/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Replaces Brad Hefta-Gaub's CounterStats class (RIP)
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__Stats__
#define __hifi__Stats__

#include <stdint.h>

class SimpleMovingAverage {
public:
    SimpleMovingAverage(int numSamplesToAverage = 100);
    
    int updateAverage(float sample);
    void reset();
    
    int getSampleCount() const { return _numSamples; };
    float getAverage() const { return _average; };
    float getEventDeltaAverage() const;
    float getAverageSampleValuePerSecond() const;
private:
    int _numSamples;
    uint64_t _lastEventTimestamp;
    float _average;
    float _eventDeltaAverage;
    
    float WEIGHTING;
    float ONE_MINUS_WEIGHTING;
};

#endif /* defined(__hifi__Stats__) */
