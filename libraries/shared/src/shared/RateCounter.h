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
#include <functional>
#include <QtCore/QElapsedTimer>

#include "../SharedUtil.h"
#include "../NumericalConstants.h"

template <uint32_t INTERVAL = MSECS_PER_SECOND, uint8_t PRECISION = 2>
class RateCounter {
public:
    void increment(size_t count = 1) {
        auto now = usecTimestampNow();
        auto currentIntervalMs = (uint32_t)((now - _start) / USECS_PER_MSEC);
        if (currentIntervalMs > INTERVAL) {
            float currentCount = _count;
            float intervalSeconds = (float)currentIntervalMs / (float)MSECS_PER_SECOND;
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
    float _rate { 0 };
    const float _scale { powf(10, PRECISION) };
};

#endif
