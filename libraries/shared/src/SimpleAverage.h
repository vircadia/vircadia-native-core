//
//  Created by Bradley Austin Davis on 2015/07/01.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_SimpleAverage_h
#define hifi_SimpleAverage_h

template<typename T>
class SimpleAverage {
public:
    void update(T sample) {
        _sum += sample;
        ++_count;
    }
    void reset() {
        _sum = 0;
        _count = 0;
    }

    int getCount() const { return _count; };
    T getAverage() const { return _sum / (T)_count; };

private:
    int _count;
    T _sum;
};

#endif
