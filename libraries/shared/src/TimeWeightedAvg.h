//
//  TimeWeightedAvg.h
//  libraries/shared/src
//
//  Created by Yixin Wang on 7/29/2014
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TimeWeightedAvg_h
#define hifi_TimeWeightedAvg_h

#include "SharedUtil.h"

template <typename T>
class TimeWeightedAvg {

public:

    TimeWeightedAvg()
        : _firstSampleTime(0),
        _lastSample(),
        _lastSampleTime(0),
        _weightedSampleSumExcludingLastSample(0.0)
    {}

    void reset() {
        _firstSampleTime = 0;
        _lastSampleTime = 0;
        _weightedSampleSumExcludingLastSample = 0.0;
    }

    void  updateWithSample(T sample) {
        quint64 now = usecTimestampNow();

        if (_firstSampleTime == 0) {
            _firstSampleTime = now;
        } else {
            _weightedSampleSumExcludingLastSample = getWeightedSampleSum(now);
        }

        _lastSample = sample;
        _lastSampleTime = now;
    }

    double getAverage() const {
        if (_firstSampleTime == 0) {
            return 0.0;
        }
        quint64 now = usecTimestampNow();
        quint64 elapsed = now - _firstSampleTime;
        return getWeightedSampleSum(now) / (double)elapsed;
    }

    quint64 getElapsedUsecs() const {
        if (_firstSampleTime == 0) {
            return 0;
        }
        return usecTimestampNow() - _firstSampleTime;
    }

private:
    // if no sample has been collected yet, the return value is undefined
    double getWeightedSampleSum(quint64 now) const {
        quint64 lastSampleLasted = now - _lastSampleTime;
        return _weightedSampleSumExcludingLastSample + (double)_lastSample * (double)lastSampleLasted;
    }

private:
    quint64 _firstSampleTime;

    T _lastSample;
    quint64 _lastSampleTime;
    
    double _weightedSampleSumExcludingLastSample;
};

#endif // hifi_TimeWeightedAvg_h
