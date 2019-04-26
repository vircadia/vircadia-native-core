//
//  RefreshRateController.h
//  libraries/display-pluging/src/display-plugin
//
//  Created by Dante Ruiz on 2019-04-15.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RefreshRateController_h
#define hifi_RefreshRateController_h

#include <DependencyManager.h>

#include <chrono>
#include <atomic>

class QThread;

class RefreshRateController {
public:
    RefreshRateController() = default;
    ~RefreshRateController() = default;

    void setRefreshRateLimitPeriod(int refreshRateLimit);
    int getRefreshRateLimitPeriod() const;

    void clockStartTime() { _startTime = std::chrono::high_resolution_clock::now(); }
    void clockEndTime() { _endTime = std::chrono::high_resolution_clock::now(); }
    void sleepThreadIfNeeded(QThread* thread, bool isHmd);
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> _startTime { std::chrono::high_resolution_clock::now() };
    std::chrono::time_point<std::chrono::high_resolution_clock> _endTime { std::chrono::high_resolution_clock::now() };
    std::atomic<int64_t> _refreshRateLimitPeriod { 50 };

};

#endif
