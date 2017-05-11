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

#include <mutex>
#include <stdint.h>
#include <atomic>

class SimpleMovingAverage {
public:
    SimpleMovingAverage(int numSamplesToAverage = 100);
    SimpleMovingAverage(const SimpleMovingAverage& other);
    SimpleMovingAverage& operator=(const SimpleMovingAverage& other);

    int updateAverage(float sample);
    void reset();

    int getSampleCount() const { return _numSamples; };
    float getAverage() const { return _average; };
    float getEventDeltaAverage() const; // returned in seconds
    float getAverageSampleValuePerSecond() const { return _average * (1.0f / getEventDeltaAverage()); }

    uint64_t getUsecsSinceLastEvent() const;


private:
    std::atomic<int> _numSamples;
    std::atomic<uint64_t> _lastEventTimestamp;
    std::atomic<float> _average;
    std::atomic<float> _eventDeltaAverage;

    float WEIGHTING;
    float ONE_MINUS_WEIGHTING;
};


template <class T, int MAX_NUM_SAMPLES> class MovingAverage {
public:
    MovingAverage<T, MAX_NUM_SAMPLES>() {}
    MovingAverage<T, MAX_NUM_SAMPLES>(const MovingAverage<T, MAX_NUM_SAMPLES>& other) {
        *this = other;
    }
    MovingAverage<T, MAX_NUM_SAMPLES>& operator=(const MovingAverage<T, MAX_NUM_SAMPLES>& other) {
        numSamples = (int)other.numSamples;
        average = (T)other.average;
        return *this;
    }

    const float WEIGHTING = 1.0f / (float)MAX_NUM_SAMPLES;
    const float ONE_MINUS_WEIGHTING = 1.0f - WEIGHTING;
    std::atomic<int> numSamples{ 0 };
    std::atomic<T> average;

    void clear() {
        numSamples = 0;
    }

    bool isAverageValid() const { return (numSamples > 0); }

    void addSample(T sample) {
        if (numSamples > 0) {
            average = (sample * (T)WEIGHTING) + (average * (T)ONE_MINUS_WEIGHTING);
        } else {
            average = sample;
        }
        numSamples++;
    }
};

template <class T, int MAX_NUM_SAMPLES> class ThreadSafeMovingAverage {
public:
    void clear() {
        std::unique_lock<std::mutex> lock(_lock);
        _samples = 0;
    }

    bool isAverageValid() const {
        std::unique_lock<std::mutex> lock(_lock);
        return (_samples > 0);
    }

    void addSample(T sample) {
        std::unique_lock<std::mutex> lock(_lock);
        if (_samples > 0) {
            _average = (sample * WEIGHTING) + (_average * ONE_MINUS_WEIGHTING);
        } else {
            _average = sample;
        }
        _samples++;
    }

    T getAverage() const {
        std::unique_lock<std::mutex> lock(_lock);
        return _average;
    }

    size_t getSamples() const {
        std::unique_lock<std::mutex> lock(_lock);
        return _samples;
    }

private:
    const float WEIGHTING = 1.0f / (float)MAX_NUM_SAMPLES;
    const float ONE_MINUS_WEIGHTING = 1.0f - WEIGHTING;
    size_t _samples { 0 };
    T _average;
    mutable std::mutex _lock;
};


#endif // hifi_SimpleMovingAverage_h
