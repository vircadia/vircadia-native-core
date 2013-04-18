//
//  SimpleMovingAverage.h
//  hifi
//
//  Created by Stephen Birarda on 4/18/13.
//  Replaces Brad Hefta-Gaub's CounterStats class (RIP)
//
//

#ifndef __hifi__Stats__
#define __hifi__Stats__

#include <iostream>

class SimpleMovingAverage {
public:
    SimpleMovingAverage(int numSamplesToAverage);
    
    int updateAverage(float sample);
    void reset();
    
    int getSampleCount() { return _numSamples; };
    float getAverage() { return _average; };
    float getEventDeltaAverage();
    float getAverageSampleValuePerSecond();
private:
    int _numSamples;
    double _lastEventTimestamp;
    float _average;
    float _eventDeltaAverage;
    
    const float WEIGHTING;
    const float ONE_MINUS_WEIGHTING;
};

#endif /* defined(__hifi__Stats__) */
