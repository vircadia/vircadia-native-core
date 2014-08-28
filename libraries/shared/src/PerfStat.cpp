//
//  PerfStat.cpp
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 3/29/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstdio>
#include <map>
#include <string>

#include <QtCore/QDebug>

#include "PerfStat.h"

#include "SharedUtil.h"

// ----------------------------------------------------------------------------
// PerformanceWarning
// ----------------------------------------------------------------------------

// Static class members initialization here!
bool PerformanceWarning::_suppressShortTimings = false;

// Destructor handles recording all of our stats
PerformanceWarning::~PerformanceWarning() {
    quint64 end = usecTimestampNow();
    quint64 elapsedusec = (end - _start);
    double elapsedmsec = elapsedusec / 1000.0;
    if ((_alwaysDisplay || _renderWarningsOn) && elapsedmsec > 1) {
        if (elapsedmsec > 1000) {
            double elapsedsec = (end - _start) / 1000000.0;
            qDebug("%s took %.2lf seconds %s", _message, elapsedsec, (_alwaysDisplay ? "" : "WARNING!") );
        } else {
            if (_suppressShortTimings) {
                if (elapsedmsec > 10) {
                    qDebug("%s took %.1lf milliseconds %s", _message, elapsedmsec,
                        (_alwaysDisplay || (elapsedmsec < 10) ? "" : "WARNING!"));
                }
            } else {
                qDebug("%s took %.2lf milliseconds %s", _message, elapsedmsec,
                    (_alwaysDisplay || (elapsedmsec < 10) ? "" : "WARNING!"));
            }
        }
    } else if (_alwaysDisplay) {
        qDebug("%s took %.2lf milliseconds", _message, elapsedmsec);
    }
    // if the caller gave us a pointer to store the running total, track it now.
    if (_runningTotal) {
        *_runningTotal += elapsedusec;
    }
    if (_totalCalls) {
        *_totalCalls += 1;
    }
};

// ----------------------------------------------------------------------------
// PerformanceTimerRecord
// ----------------------------------------------------------------------------
const quint64 STALE_STAT_PERIOD = 4 * USECS_PER_SECOND;

void PerformanceTimerRecord::tallyResult(const quint64& now) { 
    if (_numAccumulations > 0) {
        _numTallies++; 
        _movingAverage.updateAverage(_runningTotal - _lastTotal); 
        _lastTotal = _runningTotal; 
        _numAccumulations = 0;
        _expiry = now + STALE_STAT_PERIOD;
    }
}

// ----------------------------------------------------------------------------
// PerformanceTimer
// ----------------------------------------------------------------------------

QString PerformanceTimer::_fullName;
QMap<QString, PerformanceTimerRecord> PerformanceTimer::_records;


PerformanceTimer::~PerformanceTimer() {
    quint64 elapsedusec = (usecTimestampNow() - _start);
    PerformanceTimerRecord& namedRecord = _records[_fullName];
    namedRecord.accumulateResult(elapsedusec);
    _fullName.resize(_fullName.size() - (_name.size() + 1));
}

// static 
void PerformanceTimer::tallyAllTimerRecords() {
    QMap<QString, PerformanceTimerRecord>::iterator recordsItr = _records.begin();
    QMap<QString, PerformanceTimerRecord>::const_iterator recordsEnd = _records.end();
    quint64 now = usecTimestampNow();
    while (recordsItr != recordsEnd) {
        recordsItr.value().tallyResult(now);
        if (recordsItr.value().isStale(now)) {
            // purge stale records
            recordsItr = _records.erase(recordsItr);
        } else {
            ++recordsItr;
        }
    }
}

void PerformanceTimer::dumpAllTimerRecords() {
    QMapIterator<QString, PerformanceTimerRecord> i(_records);
    while (i.hasNext()) {
        i.next();
        qDebug() << i.key() << ": average " << i.value().getAverage() 
            << " [" << i.value().getMovingAverage() << "]"
            << "usecs over" << i.value().getCount() << "calls";
    }
}
