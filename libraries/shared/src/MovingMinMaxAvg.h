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
    
    MovingMinMaxAvg(int intervalLength, int windowIntervals)
        : _min(std::numeric_limits<T>::max()),
        _max(std::numeric_limits<T>::min()),
        _average(0.0),
        _samplesCollected(0),
        _intervalLength(intervalLength),
        _windowIntervals(windowIntervals),
        _existingSamplesInCurrentInterval(0),
        _existingIntervals(0),
        _windowMin(std::numeric_limits<T>::max()),
        _windowMax(std::numeric_limits<T>::min()),
        _windowAverage(0.0),
        _currentIntervalMin(std::numeric_limits<T>::max()),
        _currentIntervalMax(std::numeric_limits<T>::min()),
        _currentIntervalAverage(0.0),
        _newestIntervalStatsAt(0),
        _newStatsAvailable(false)
    {
        _intervalMins = new T[_windowIntervals];
        _intervalMaxes = new T[_windowIntervals];
        _intervalAverages = new double[_windowIntervals];
    }

    ~MovingMinMaxAvg() {
        delete[] _intervalMins;
        delete[] _intervalMaxes;
        delete[] _intervalAverages;
    }
    
    void reset() {
        _min = std::numeric_limits<T>::max();
        _max = std::numeric_limits<T>::min();
        _average = 0.0;
        _samplesCollected = 0;
        _existingSamplesInCurrentInterval = 0;
        _existingIntervals = 0;
        _windowMin = std::numeric_limits<T>::max();
        _windowMax = std::numeric_limits<T>::min();
        _windowAverage = 0.0;
        _currentIntervalMin = std::numeric_limits<T>::max();
        _currentIntervalMax = std::numeric_limits<T>::min();
        _currentIntervalAverage = 0.0;
        _newStatsAvailableFlag = false;
    }

    void update(T newSample) {

        // update overall stats
        if (newSample < _min) {
            _min = newSample;
        }
        if (newSample > _max) {
            _max = newSample;
        }
        updateAverage(_average, _samplesCollected, (double)newSample);

        // update the current interval stats
        if (newSample < _currentIntervalMin) {
            _currentIntervalMin = newSample;
        }
        if (newSample > _currentIntervalMax) {
            _currentIntervalMax = newSample;
        }
        updateAverage(_currentIntervalAverage, _existingSamplesInCurrentInterval, (double)newSample);

        // if the current interval of samples is now full, record its stats into our past intervals' stats
        if (_existingSamplesInCurrentInterval == _intervalLength) {

            // increment index of the newest interval's stats cyclically
            _newestIntervalStatsAt = _newestIntervalStatsAt == _windowIntervals - 1 ? 0 : _newestIntervalStatsAt + 1;

            // record current interval's stats, then reset them
            _intervalMins[_newestIntervalStatsAt] = _currentIntervalMin;
            _intervalMaxes[_newestIntervalStatsAt] = _currentIntervalMax;
            _intervalAverages[_newestIntervalStatsAt] = _currentIntervalAverage;
            _currentIntervalMin = std::numeric_limits<T>::max();
            _currentIntervalMax = std::numeric_limits<T>::min();
            _currentIntervalAverage = 0.0;
            _existingSamplesInCurrentInterval = 0;

            if (_existingIntervals < _windowIntervals) {
                _existingIntervals++;
            }

            // update the window's stats
            int k = _newestIntervalStatsAt;
            _windowMin = _intervalMins[k];
            _windowMax = _intervalMaxes[k];
            _windowAverage = _intervalAverages[k];
            int intervalsIncludedInWindowStats = 1;
            while (intervalsIncludedInWindowStats < _existingIntervals) {
                k = k == 0 ? _windowIntervals - 1 : k - 1;
                if (_intervalMins[k] < _windowMin) {
                    _windowMin = _intervalMins[k];
                }
                if (_intervalMaxes[k] > _windowMax) {
                    _windowMax = _intervalMaxes[k];
                }
                updateAverage(_windowAverage, intervalsIncludedInWindowStats, _intervalAverages[k]);
            }

            _newStatsAvailable = true;
        }
    }


    bool getNewStatsAvailableFlag() const { return _newStatsAvailable; }
    void clearNewStatsAvailableFlag() { _newStatsAvailable = false; }

    T getMin() const { return _min; }
    T getMax() const { return _max; }
    double getAverage() const { return _average; }
    T getWindowMin() const { return _windowMin; }
    T getWindowMax() const { return _windowMax; }
    double getWindowAverage() const { return _windowAverage; }

private:
    void updateAverage(double& average, int& numSamples, double newSample) {
        // update some running average without overflowing it
        average = average * ((double)numSamples / (numSamples + 1)) + newSample / (numSamples + 1);
        numSamples++;
    }

private:
    // these are min/max/avg stats for all samples collected.
    T _min;
    T _max;
    double _average;
    int _samplesCollected;

    int _intervalLength;
    int _windowIntervals;

    int _existingSamplesInCurrentInterval;
    int _existingIntervals;

    // these are the min/max/avg stats for the samples in the moving window
    T _windowMin;
    T _windowMax;
    double _windowAverage;

    T _currentIntervalMin;
    T _currentIntervalMax;
    double _currentIntervalAverage;

    T* _intervalMins;
    T* _intervalMaxes;
    double* _intervalAverages;
    int _newestIntervalStatsAt;

    bool _newStatsAvailable;
};

#endif // hifi_OctalCode_h
