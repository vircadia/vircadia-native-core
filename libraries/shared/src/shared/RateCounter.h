//
//  Created by Bradley Austin Davis 2016/04/10
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Shared_RateCounter_h
#define hifi_Shared_RateCounter_h

#include <stdint.h>
#include <atomic>
#include <functional>
#include <QtCore/QElapsedTimer>

#include "../SharedUtil.h"
#include "../NumericalConstants.h"

template <uint32_t INTERVAL = MSECS_PER_SECOND, uint8_t PRECISION = 2>
class RateCounter {
public:
    RateCounter() { _rate = 0; } // avoid use of std::atomic copy ctor

    void increment(size_t count = 1) {
        auto now = usecTimestampNow();
        float currentIntervalMs = (now - _start) / (float) USECS_PER_MSEC;
        if (currentIntervalMs > (float) INTERVAL) {
            float currentCount = _count;
            float intervalSeconds = currentIntervalMs / (float) MSECS_PER_SECOND;
            _rate = roundf(currentCount / intervalSeconds * _scale) / _scale;
            _start = now;
            _count = 0;
        };
        _count += count;
    }

    float rate() const { return _rate; }

    uint8_t precision() const { return PRECISION; }

    uint32_t interval() const { return INTERVAL; }

private:
    uint64_t _start { usecTimestampNow() };
    size_t _count { 0 };
    const float _scale { powf(10, PRECISION) };
    std::atomic<float> _rate;
};

#endif
