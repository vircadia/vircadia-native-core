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
        checkRate();
        _count += count;
    }

    float rate() const { checkRate(); return _rate; }

    uint8_t precision() const { return PRECISION; }

    uint32_t interval() const { return INTERVAL; }

private:
    mutable uint64_t _expiry { usecTimestampNow() + INTERVAL * USECS_PER_MSEC};
    mutable size_t _count { 0 };
    const float _scale { powf(10, PRECISION) };
    mutable std::atomic<float> _rate;

    void checkRate() const {
        auto now = usecTimestampNow();
        if (now > _expiry) {
            float MSECS_PER_USEC = 0.001f;
            float SECS_PER_MSEC = 0.001f;
            float intervalSeconds = ((float)INTERVAL + (float)(now - _expiry) * MSECS_PER_USEC) * SECS_PER_MSEC;
            _rate = roundf((float)_count / intervalSeconds * _scale) / _scale;
            _expiry = now + INTERVAL * USECS_PER_MSEC;
            _count = 0;
        };
    }

};

#endif
