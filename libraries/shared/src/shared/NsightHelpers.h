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

#if defined(NSIGHT_FOUND)
    class ProfileRange {
    public:
        ProfileRange(const char *name);
        ~ProfileRange();
    };
#define PROFILE_RANGE(name) ProfileRange profileRangeThis(name);
#else
#define PROFILE_RANGE(name)
#endif


#endif