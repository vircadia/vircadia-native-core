//
//  RefreshRateController.cpp
//  libraries/display-pluging/src/display-plugin
//
//  Created by Dante Ruiz on 2019-04-15.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RefreshRateController.h"

#include <QtCore/QThread>
#include <NumericalConstants.h>

long int hzToDurationNanoseconds(int refreshRate) {
    return (int64_t) (NSECS_PER_SECOND / (quint64) refreshRate);
}

int durationNanosecondsToHz(int64_t refreshRateLimitPeriod) {
    return (int) (NSECS_PER_SECOND / (quint64) refreshRateLimitPeriod);
}

void RefreshRateController::setRefreshRateLimitPeriod(int refreshRateLimit) {
    _refreshRateLimitPeriod = hzToDurationNanoseconds(refreshRateLimit);
}

int RefreshRateController::getRefreshRateLimitPeriod() const {
    return durationNanosecondsToHz(_refreshRateLimitPeriod);
}

void RefreshRateController::sleepThreadIfNeeded(QThread* thread, bool isHmd) {
    if (!isHmd) {
        static const std::chrono::nanoseconds EPSILON = std::chrono::milliseconds(1);
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(_endTime - _startTime);
        auto refreshRateLimitPeriod = std::chrono::nanoseconds(_refreshRateLimitPeriod);
        auto sleepDuration = refreshRateLimitPeriod - (duration + EPSILON);
        if (sleepDuration.count() > 0) {
            thread->msleep(std::chrono::duration_cast<std::chrono::milliseconds>(sleepDuration).count());
        }
    }
}
