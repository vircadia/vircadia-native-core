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

#include <cassert>
#include <limits>

#include "RingBufferHistory.h"

template <typename T>
class MinMaxAvg {
public:
    MinMaxAvg()
        : _min(std::numeric_limits<T>::max()),
        _max(std::numeric_limits<T>::min()),
        _average(0.0),
        _samples(0),
        _last(0)
    {}

    void reset() {
        _min = std::numeric_limits<T>::max();
        _max = std::numeric_limits<T>::min();
        _average = 0.0;
        _samples = 0;
        _last = 0;
    }

    void update(T sample) {
        if (sample < _min) {
            _min = sample;
        }
        if (sample > _max) {
            _max = sample;
        }
        double totalSamples = _samples + 1;
        _average = _average * ((double)_samples / totalSamples)
            + (double)sample / totalSamples;
        _samples++;
        
        _last = sample;
    }

    void update(const MinMaxAvg<T>& other) {
        if (other._min < _min) {
            _min = other._min;
        }
        if (other._max > _max) {
            _max = other._max;
        }
        double totalSamples = _samples + other._samples;
        _average = _average * ((double)_samples / totalSamples)
            + other._average * ((double)other._samples / totalSamples);
        _samples += other._samples;
    }

    T getMin() const { return _min; }
    T getMax() const { return _max; }
    double getAverage() const { return _average; }
    int getSamples() const { return _samples; }
    double getSum() const { return _samples * _average; }
    T getLast() const { return _last; }

private:
    T _min;
    T _max;
    double _average;
    int _samples;
    T _last;
};

template <typename T>
class MovingMinMaxAvg {
public:
    // This class collects 3 stats (min, max, avg) over a moving window of samples.
    // The moving window contains _windowIntervals * _intervalLength samples.
    // Those stats are updated every _intervalLength samples collected.  When that happens, _newStatsAvaialble is set
    // to true and it's up to the user to clear that flag.
    // For example, if you want a moving avg of the past 5000 samples updated every 100 samples, you would instantiate
    // this class with MovingMinMaxAvg(100, 50).  If you want a moving min of the past 100 samples updated on every
    // new sample, instantiate this class with MovingMinMaxAvg(1, 100).
    

    /// use intervalLength = 0 to use in manual mode, where the currentIntervalComplete() function must
    /// be called to complete an interval
    MovingMinMaxAvg(int intervalLength, int windowIntervals)
        : _intervalLength(intervalLength),
        _windowIntervals(windowIntervals),
        _overallStats(),
        _windowStats(),
        _currentIntervalStats(),
        _intervalStats(windowIntervals),
        _newStatsAvailable(false)
    {}
    
    void reset() {
        _overallStats.reset();
        _windowStats.reset();
        _currentIntervalStats.reset();
        _intervalStats.clear();
        _newStatsAvailable = false;
    }

    void setWindowIntervals(int windowIntervals) {
        _windowIntervals = windowIntervals;
        _overallStats.reset();
        _windowStats.reset();
        _currentIntervalStats.reset();
        _intervalStats.setCapacity(_windowIntervals);
        _newStatsAvailable = false;
    }

    void update(T newSample) {
        // update overall stats
        _overallStats.update(newSample);

        // update the current interval stats
        _currentIntervalStats.update(newSample);

        // if the current interval of samples is now full, record its stats into our past intervals' stats
        // NOTE: if _intervalLength is 0 (manual mode), currentIntervalComplete() will not be called here.
        if (_currentIntervalStats.getSamples() == _intervalLength) {
            currentIntervalComplete();
        }
    }

    /// This function can be called to manually control when each interval ends.  For example, if each interval
    /// needs to last T seconds as opposed to N samples, this function should be called every T seconds.
    void currentIntervalComplete() {
        // record current interval's stats, then reset them
        _intervalStats.insert(_currentIntervalStats);
        _currentIntervalStats.reset();

        // update the window's stats by combining the intervals' stats
        typename RingBufferHistory< MinMaxAvg<T> >::Iterator i = _intervalStats.begin();
        typename RingBufferHistory< MinMaxAvg<T> >::Iterator end = _intervalStats.end();
        _windowStats.reset();
        while (i != end) {
            _windowStats.update(*i);
            ++i;
        }

        _newStatsAvailable = true;
    }

    bool getNewStatsAvailableFlag() const { return _newStatsAvailable; }
    void clearNewStatsAvailableFlag() { _newStatsAvailable = false; }

    T getMin() const { return _overallStats.getMin(); }
    T getMax() const { return _overallStats.getMax(); }
    double getAverage() const { return _overallStats.getAverage(); }
    int getSamples() const { return _overallStats.getSamples(); }
    double getSum() const { return _overallStats.getSum(); }

    T getWindowMin() const { return _windowStats.getMin(); }
    T getWindowMax() const { return _windowStats.getMax(); }
    double getWindowAverage() const { return _windowStats.getAverage(); }
    int getWindowSamples() const { return _windowStats.getSamples(); }
    double getWindowSum() const { return _windowStats.getSum(); }

    int getWindowIntervals() const { return _windowIntervals; }
    T getCurrentIntervalMin() const { return _currentIntervalStats.getMin(); }
    T getCurrentIntervalMax() const { return _currentIntervalStats.getMax(); }
    double getCurrentIntervalAverage() const { return _currentIntervalStats.getAverage(); }
    int getCurrentIntervalSamples() const { return _currentIntervalStats.getSamples(); }
    double getCurrentIntervalSum() const { return _currentIntervalStats.getSum(); }
    T getCurrentIntervalLastSample() const { return _currentIntervalStats.getLast(); }
    
    const MinMaxAvg<T>& getOverallStats() const{ return _overallStats; }
    const MinMaxAvg<T>& getWindowStats() const{ return _windowStats; }
    const MinMaxAvg<T>& getCurrentIntervalStats() const { return _currentIntervalStats; }
    
    MinMaxAvg<T> getLastCompleteIntervalStats() const {
        const MinMaxAvg<T>* stats = _intervalStats.getNewestEntry();
        return stats == NULL ? MinMaxAvg<T>() : *stats;
    }

    bool isWindowFilled() const { return _intervalStats.isFilled(); }

private:
    int _intervalLength;
    int _windowIntervals;

    // these are min/max/avg stats for all samples collected.
    MinMaxAvg<T> _overallStats;

    // these are the min/max/avg stats for the samples in the moving window
    MinMaxAvg<T> _windowStats;

    // these are the min/max/avg stats for the samples in the current interval
    MinMaxAvg<T> _currentIntervalStats;

    // these are stored stats for the past intervals in the window
    RingBufferHistory< MinMaxAvg<T> > _intervalStats;

    bool _newStatsAvailable;
};

#endif // hifi_MovingMinMaxAvg_h
