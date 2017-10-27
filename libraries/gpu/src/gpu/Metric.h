//
//  Metric.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 5/17/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Metric_h
#define hifi_gpu_Metric_h

#include "Forward.h"

namespace gpu {

template <typename T>
class Metric {
    std::atomic<T> _value { 0 };
    std::atomic<T> _maximum { 0 };

public:
    T getValue() { return _value; }
    T getMaximum() { return _maximum; }

    void set(T newValue) {
        _value = newValue;
    }
    void increment() {
        auto total = ++_value;
        if (total > _maximum.load()) {
            _maximum = total;
        }
    }
    void decrement() {
        --_value;
    }

    void update(T prevValue, T newValue) {
        if (prevValue == newValue) {
            return;
        }
        if (newValue > prevValue) {
            auto total = _value.fetch_add(newValue - prevValue);
            if (total > _maximum.load()) {
                _maximum = total;
            }
        } else {
            _value.fetch_sub(prevValue - newValue);
        }
    }

    void reset() {
        _value = 0;
        _maximum = 0;
    }
};

using ContextMetricCount = Metric<uint32_t>;
using ContextMetricSize = Metric<Size>;

}
#endif
