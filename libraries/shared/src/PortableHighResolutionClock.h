//
//  PortableHighResolutionClock.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2015-09-14.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//  I discovered the low-resolution nature of the VC2012 implementation of std::chrono::high_resolution_clock
//  while debugging a UDT issue with packet receive speeds. That lead to
//  http://stackoverflow.com/questions/16299029/resolution-of-stdchronohigh-resolution-clock-doesnt-correspond-to-measureme
//  which is where the implementation of this class is from.

#pragma once

#ifndef hifi_PortableHighResolutionClock_h
#define hifi_PortableHighResolutionClock_h

#include <chrono>

#include <QtCore/QMetaType>

#if defined(_MSC_VER) && _MSC_VER < 1900

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// The following struct is not compliant with the HF coding standard, but uses underscores to match the classes
// in std::chrono

struct win_high_resolution_clock {
    typedef long long rep;
    typedef std::nano period;
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<win_high_resolution_clock> time_point;
    static const bool is_steady = true;
    
    static time_point now();
};

using p_high_resolution_clock = win_high_resolution_clock;

#else

using p_high_resolution_clock = std::chrono::high_resolution_clock;

#endif

Q_DECLARE_METATYPE(p_high_resolution_clock::time_point);

static const int timePointMetaTypeID = qRegisterMetaType<p_high_resolution_clock::time_point>();

#endif // hifi_PortableHighResolutionClock_h
