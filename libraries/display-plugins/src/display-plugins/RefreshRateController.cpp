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

int refreshRateConverter(int refreshRate) {
    const float ONE_SECOND_IN_MILLISECONDS = 1000.0f;
    const float ONE_TIME_UNIT = 1.0f;
    float frameInOneTimeUnit = ONE_TIME_UNIT / (float) refreshRate;
    float convertedRefreshRate = frameInOneTimeUnit * ONE_SECOND_IN_MILLISECONDS;
    return (int) convertedRefreshRate;
}

void RefreshRateController::setRefreshRateLimit(int refreshRateLimit) {
    _refreshRateLimit = refreshRateConverter(refreshRateLimit);
}

int RefreshRateController::getRefreshRateLimit() const {
    return refreshRateConverter(_refreshRateLimit);
}

void RefreshRateController::sleepThreadIfNeeded(QThread* thread) {
    auto startTimeFromEpoch = _startTime.time_since_epoch();
    auto endTimeFromEpoch = _endTime.time_since_epoch();

    auto startMs = std::chrono::duration_cast<std::chrono::milliseconds>(startTimeFromEpoch).count();
    auto endMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTimeFromEpoch).count();
    auto duration = endMs - startMs;
    if (duration < _refreshRateLimit) {
        thread->msleep(_refreshRateLimit - duration);
    }
}
