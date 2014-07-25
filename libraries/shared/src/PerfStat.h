//
//  PerfStat.h
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 3/29/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Poor-man's performance stats collector class. Useful for collecting timing
//  details from various portions of the code.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PerfStat_h
#define hifi_PerfStat_h

#include <stdint.h>
#include "SharedUtil.h"
#include "SimpleMovingAverage.h"

#include <cstring>
#include <string>
#include <map>

class PerformanceWarning {
private:
    quint64 _start;
    const char* _message;
    bool _renderWarningsOn;
    bool _alwaysDisplay;
    quint64* _runningTotal;
    quint64* _totalCalls;
    static bool _suppressShortTimings;
public:

    PerformanceWarning(bool renderWarnings, const char* message, bool alwaysDisplay = false,
                        quint64* runningTotal = NULL, quint64* totalCalls = NULL) :
        _start(usecTimestampNow()),
        _message(message),
        _renderWarningsOn(renderWarnings),
        _alwaysDisplay(alwaysDisplay),
        _runningTotal(runningTotal),
        _totalCalls(totalCalls) { }
        
    quint64 elapsed() const { return (usecTimestampNow() - _start); };

    ~PerformanceWarning();

    static void setSuppressShortTimings(bool suppressShortTimings) { _suppressShortTimings = suppressShortTimings; }
};

class PerformanceTimerRecord {
public:
    PerformanceTimerRecord() : _runningTotal(0), _lastTotal(0), _numAccumulations(0), _numTallies(0), _expiry(0) {}
    
    void accumulateResult(const quint64& elapsed) { _runningTotal += elapsed; ++_numAccumulations; }
    void tallyResult(const quint64& now);
    bool isStale(const quint64& now) const { return now > _expiry; }
    quint64 getAverage() const { return (_numTallies == 0) ? 0 : _runningTotal / _numTallies; }
    quint64 getMovingAverage() const { return (_numTallies == 0) ? 0 : _movingAverage.getAverage(); }
    quint64 getCount() const { return _numTallies; }
    
private:
    quint64 _runningTotal;
    quint64 _lastTotal;
    quint64 _numAccumulations;
    quint64 _numTallies;
    quint64 _expiry;
    SimpleMovingAverage _movingAverage;
};

class PerformanceTimer {
public:

    PerformanceTimer(const QString& name) :
        _start(0),
        _name(name) {
            _fullName.append("/");
            _fullName.append(_name);
            _start = usecTimestampNow();
        }
        
    ~PerformanceTimer();
    
    static const PerformanceTimerRecord& getTimerRecord(const QString& name) { return _records[name]; };
    static const QMap<QString, PerformanceTimerRecord>& getAllTimerRecords() { return _records; };
    static void tallyAllTimerRecords();
    static void dumpAllTimerRecords();

private:
    quint64 _start;
    QString _name;
    static QString _fullName;
    static QMap<QString, PerformanceTimerRecord> _records;
};


#endif // hifi_PerfStat_h
