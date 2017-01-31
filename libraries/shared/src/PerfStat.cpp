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

#include <QDebug>
#include <QThread>

#include "PerfStat.h"

#include "NumericalConstants.h"
#include "SharedLogging.h"

// ----------------------------------------------------------------------------
// PerformanceWarning
// ----------------------------------------------------------------------------

// Static class members initialization here!
bool PerformanceWarning::_suppressShortTimings = false;

// Destructor handles recording all of our stats
PerformanceWarning::~PerformanceWarning() {
    quint64 end = usecTimestampNow();
    quint64 elapsedUsec = (end - _start);
    double elapsedmsec = elapsedUsec / 1000.0;
    if ((_alwaysDisplay || _renderWarningsOn) && elapsedmsec > 1) {
        if (elapsedmsec > 1000) {
            double elapsedsec = (end - _start) / 1000000.0;
            qCDebug(shared, "%s took %.2lf seconds %s", _message, elapsedsec, (_alwaysDisplay ? "" : "WARNING!") );
        } else {
            if (_suppressShortTimings) {
                if (elapsedmsec > 10) {
                    qCDebug(shared, "%s took %.1lf milliseconds %s", _message, elapsedmsec,
                        (_alwaysDisplay || (elapsedmsec < 10) ? "" : "WARNING!"));
                }
            } else {
                qCDebug(shared, "%s took %.2lf milliseconds %s", _message, elapsedmsec,
                    (_alwaysDisplay || (elapsedmsec < 10) ? "" : "WARNING!"));
            }
        }
    } else if (_alwaysDisplay) {
        qCDebug(shared, "%s took %.2lf milliseconds", _message, elapsedmsec);
    }
    // if the caller gave us a pointer to store the running total, track it now.
    if (_runningTotal) {
        *_runningTotal += elapsedUsec;
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

std::atomic<bool> PerformanceTimer::_isActive(false);
QHash<QThread*, QString> PerformanceTimer::_fullNames;
QMap<QString, PerformanceTimerRecord> PerformanceTimer::_records;


PerformanceTimer::PerformanceTimer(const QString& name) {
    if (_isActive) {
        _name = name;
        QString& fullName = _fullNames[QThread::currentThread()];
        fullName.append("/");
        fullName.append(_name);
        _start = usecTimestampNow();
    }
}

PerformanceTimer::~PerformanceTimer() {
    if (_isActive && _start != 0) {
        quint64 elapsedUsec = (usecTimestampNow() - _start);
        QString& fullName = _fullNames[QThread::currentThread()];
        PerformanceTimerRecord& namedRecord = _records[fullName];
        namedRecord.accumulateResult(elapsedUsec);
        fullName.resize(fullName.size() - (_name.size() + 1));
    }
}

// static
bool PerformanceTimer::isActive() {
    return _isActive;
}

// static
QString PerformanceTimer::getContextName() {
    return _fullNames[QThread::currentThread()];
}

// static
void PerformanceTimer::addTimerRecord(const QString& fullName, quint64 elapsedUsec) {
    PerformanceTimerRecord& namedRecord = _records[fullName];
    namedRecord.accumulateResult(elapsedUsec);
}

// static
void PerformanceTimer::setActive(bool active) {
    if (active != _isActive) {
        _isActive.store(active);
        if (!active) {
            _fullNames.clear();
            _records.clear();
        }

        qCDebug(shared) << "PerformanceTimer has been turned" << ((active) ? "on" : "off");
    }
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
        qCDebug(shared) << i.key() << ": average " << i.value().getAverage()
            << " [" << i.value().getMovingAverage() << "]"
            << "usecs over" << i.value().getCount() << "calls";
    }
}
