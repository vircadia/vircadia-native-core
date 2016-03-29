//
//  PortableHighResolutionClock.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2015-09-14.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#if defined(_MSC_VER) && _MSC_VER < 1900

#include "PortableHighResolutionClock.h"

namespace {
    const long long g_Frequency = []() -> long long {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        return frequency.QuadPart;
    }();
}

win_high_resolution_clock::time_point win_high_resolution_clock::now() {
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return time_point(duration(static_cast<rep>((double) count.QuadPart * (double) period::den / (double)g_Frequency)));
}

#endif
