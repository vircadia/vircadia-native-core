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

#ifndef hifi_SimpleMovingAverage_h
#define hifi_SimpleMovingAverage_h

#include <stdint.h>
#include <list>

class SimpleMovingAverage {
public:
    SimpleMovingAverage(int numSamplesToAverage = 100);
    
    int updateAverage(float sample);
    void reset();
    
    int getSampleCount() const { return _numSamples; };
    float getAverage() const { return _average; };
    float getEventDeltaAverage() const; // returned in seconds
    float getAverageSampleValuePerSecond() const { return _average * (1.0f / getEventDeltaAverage()); }
    
    uint64_t getUsecsSinceLastEvent() const;

private:
    int _numSamples;
    uint64_t _lastEventTimestamp;
    float _average;
    float _eventDeltaAverage;
    
    float WEIGHTING;
    float ONE_MINUS_WEIGHTING;
};


template <class T, int MAX_NUM_SAMPLES> class MovingAverage {
public:
    using Samples = std::list< T >;
    Samples samples;
    T average;

    void clear() {
        samples.clear();
    }

    bool isAverageValid() const { return !samples.empty(); }

    void addSample(T sample) {
        samples.push_front(sample);
        int numSamples = samples.size();

        if (numSamples < MAX_NUM_SAMPLES) {
            average = (sample + average * (float)(numSamples - 1)) / (float)(numSamples);
        } else {
            T tail = samples.back();
            samples.pop_back();
            average = average + (sample - tail) / (float)(numSamples);
        }
    }
};

#endif // hifi_SimpleMovingAverage_h
