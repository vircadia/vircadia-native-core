//
//  MovingMinMaxAvg.h
//  libraries/shared/src
//
//  Created by Yixin Wang on 7/8/2014
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MovingMinMaxAvg_h
#define hifi_MovingMinMaxAvg_h

#include <limits>

#include "RingBufferHistory.h"

template <typename T>
class MovingMinMaxAvg {

private:
    class Stats {
    public:
        Stats()
            : _min(std::numeric_limits<T>::max()),
            _max(std::numeric_limits<T>::min()),
            _average(0.0) {}

        void updateWithSample(T sample, int& numSamplesInAverage) {
            if (sample < _min) {
                _min = sample;
            }
            if (sample > _max) {
                _max = sample;
            }
            _average = _average * ((double)numSamplesInAverage / (numSamplesInAverage + 1))
                + (double)sample / (numSamplesInAverage + 1);
            numSamplesInAverage++;
        }

        void updateWithOtherStats(const Stats& other, int& numStatsInAverage) {
            if (other._min < _min) {
                _min = other._min;
            }
            if (other._max > _max) {
                _max = other._max;
            }
            _average = _average * ((double)numStatsInAverage / (numStatsInAverage + 1))
                + other._average / (numStatsInAverage + 1);
            numStatsInAverage++;
        }

        T _min;
        T _max;
        double _average;
    };

public:
    // This class collects 3 stats (min, max, avg) over a moving window of samples.
    // The moving window contains _windowIntervals * _intervalLength samples.
    // Those stats are updated every _intervalLength samples collected.  When that happens, _newStatsAvaialble is set
    // to true and it's up to the user to clear that flag.
    // For example, if you want a moving avg of the past 5000 samples updated every 100 samples, you would instantiate
    // this class with MovingMinMaxAvg(100, 50).  If you want a moving min of the past 100 samples updated on every
    // new sample, instantiate this class with MovingMinMaxAvg(1, 100).
    
    MovingMinMaxAvg(int intervalLength, int windowIntervals)
        : _intervalLength(intervalLength),
        _windowIntervals(windowIntervals),
        _overallStats(),
        _samplesCollected(0),
        _windowStats(),
        _existingSamplesInCurrentInterval(0),
        _currentIntervalStats(),
        _intervalStats(windowIntervals),
        _newStatsAvailable(false)
    {}
    
    void reset() {
        _overallStats = Stats();
        _samplesCollected = 0;
        _windowStats = Stats();
        _existingSamplesInCurrentInterval = 0;
        _currentIntervalStats = Stats();
        _intervalStats.clear();
        _newStatsAvailable = false;
    }

    void update(T newSample) {
        // update overall stats
        _overallStats.updateWithSample(newSample, _samplesCollected);

        // update the current interval stats
        _currentIntervalStats.updateWithSample(newSample, _existingSamplesInCurrentInterval);

        // if the current interval of samples is now full, record its stats into our past intervals' stats
        if (_existingSamplesInCurrentInterval == _intervalLength) {

            // record current interval's stats, then reset them
            _intervalStats.insert(_currentIntervalStats);
            _currentIntervalStats = Stats();
            _existingSamplesInCurrentInterval = 0;

            // update the window's stats by combining the intervals' stats
            typename RingBufferHistory<Stats>::Iterator i = _intervalStats.begin();
            typename RingBufferHistory<Stats>::Iterator end = _intervalStats.end();
            _windowStats = Stats();
            int intervalsIncludedInWindowStats = 0;
            while (i != end) {
                _windowStats.updateWithOtherStats(*i, intervalsIncludedInWindowStats);
                i++;
            }

            _newStatsAvailable = true;
        }
    }

    bool getNewStatsAvailableFlag() const { return _newStatsAvailable; }
    void clearNewStatsAvailableFlag() { _newStatsAvailable = false; }

    T getMin() const { return _overallStats._min; }
    T getMax() const { return _overallStats._max; }
    double getAverage() const { return _overallStats._average; }
    T getWindowMin() const { return _windowStats._min; }
    T getWindowMax() const { return _windowStats._max; }
    double getWindowAverage() const { return _windowStats._average; }

    bool isWindowFilled() const { return _intervalStats.isFilled(); }

private:
    int _intervalLength;
    int _windowIntervals;

    // these are min/max/avg stats for all samples collected.
    Stats _overallStats;
    int _samplesCollected;

    // these are the min/max/avg stats for the samples in the moving window
    Stats _windowStats;
    int _existingSamplesInCurrentInterval;

    // these are the min/max/avg stats for the current interval
    Stats _currentIntervalStats;

    // these are stored stats for the past intervals in the window
    RingBufferHistory<Stats> _intervalStats;

    bool _newStatsAvailable;
};

#endif // hifi_MovingMinMaxAvg_h
