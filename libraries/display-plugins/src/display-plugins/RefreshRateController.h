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

    void setRefreshRateLimit(int refreshRateLimiti);
    int getRefreshRateLimit() const;

    void clockStartTime() { _startTime = std::chrono::system_clock::now(); }
    void clockEndTime() { _endTime = std::chrono::system_clock::now(); }
    void sleepThreadIfNeeded(QThread* thread);
private:

    std::chrono::time_point<std::chrono::system_clock> _startTime { std::chrono::system_clock::now() };
    std::chrono::time_point<std::chrono::system_clock> _endTime { std::chrono::system_clock::now() };
    std::atomic_int _refreshRateLimit { 50 }; // milliseconds

};

#endif
