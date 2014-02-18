//
//  HiFiPerfStat.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  Poor-man's performance stats collector class. Useful for collecting timing
//  details from various portions of the code.
//
//

#ifndef __hifi__PerfStat__
#define __hifi__PerfStat__

#include <stdint.h>
#include "SharedUtil.h"

#ifdef _WIN32
#include "Systime.h"
#else
#include <sys/time.h>
#endif

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


#endif /* defined(__hifi__PerfStat__) */
