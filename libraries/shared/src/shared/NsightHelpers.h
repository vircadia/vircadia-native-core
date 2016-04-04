//
//  Created by Bradley Austin Davis on 2015/12/10
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gl_NsightHelpers_h
#define hifi_gl_NsightHelpers_h

#ifdef _WIN32
#include <stdint.h>

class ProfileRange {
public:
    ProfileRange(const char *name);
    ProfileRange(const char *name, uint32_t argbColor, uint64_t payload);
    ~ProfileRange();
};

#define PROFILE_RANGE(name) ProfileRange profileRangeThis(name);
#define PROFILE_RANGE_EX(name, argbColor, payload) ProfileRange profileRangeThis(name, argbColor, (uint64_t)payload);
#else
#define PROFILE_RANGE(name)
#define PROFILE_RANGE_EX(name, argbColor, payload)
#endif

#endif
