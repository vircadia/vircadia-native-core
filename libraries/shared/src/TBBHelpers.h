//
//  Created by Bradley Austin Davis on 2017/06/06
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_TBBHelpers_h
#define hifi_TBBHelpers_h

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4334 )
#endif

#if !defined(Q_MOC_RUN)
// Work around https://bugreports.qt.io/browse/QTBUG-80990
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/concurrent_vector.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range2d.h>
#endif

#ifdef _WIN32
#pragma warning( pop )
#endif

#endif // hifi_TBBHelpers_h
