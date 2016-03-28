//
//  Created by Bradley Austin Davis on 2015/09/01
//  Copyright 2013-2105 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Simulates a java finally block by executing a lambda when an instance leaves
// scope

#include <functional>

#pragma once
#ifndef hifi_Finally_h
#define hifi_Finally_h

class Finally {
public:
    template <typename F>
    Finally(F f) : _f(f) {}
    ~Finally() { _f(); }
    void trigger() {
        _f();
        _f = [] {};
    }
private:
    std::function<void()> _f;
};

#endif
