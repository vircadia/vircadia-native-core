//
//  Created by Bradley Austin Davis on 2016/04/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <stdint.h>
#include <functional>

#include "../SharedUtil.h"
#include "../NumericalConstants.h"

template <size_t MS = 1000>
class OnceEvery {
public:
    OnceEvery(std::function<void()> f) : _f(f) { }

    bool maybeExecute() {
        uint64_t now = usecTimestampNow();
        if ((now - _lastRun) > (MS * USECS_PER_MSEC)) {
            _f();
            _lastRun = now;
            return true;
        }
        return false;
    }

private:
    uint64_t _lastRun { 0 };
    std::function<void()> _f;
};
