//
//  HiFiPerfStat.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Poor-man's performance stats collector class. Useful for collecting timing
//  details from various portions of the code.
//
//

#include <cstdio>
#include <map>
#include <string>

#include <QtCore/QDebug>

#include "PerfStat.h"

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


