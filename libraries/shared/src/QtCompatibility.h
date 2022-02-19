//
//  QtCompatibility.h
//
//  Created by Julian Groß on 2022-02-04
//  Copyright 2022 Overte e.V.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGlobal>

// Compatibility with Qt < 5.13
#ifndef Q_DISABLE_COPY
    #define Q_DISABLE_COPY(className) \
        className(const className &) = delete;\
        className &operator=(const className &) = delete;
#endif

// Compatibility with Qt < 5.13
#ifndef Q_DISABLE_COPY_MOVE
    #define Q_DISABLE_COPY_MOVE(className) \
        className(className & other) = delete;\
        className(className && other) = delete;
#endif

// Compatibility with Qt < 5.15
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    #define QTCOMPAT_ENDL endl
#else
    #define QTCOMPAT_ENDL Qt::endl
#endif

// Compatibility with Qt < 5.15
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    #define QTCOMPAT_KEEP_EMPTY_PARTS QString::KeepEmptyParts
#else
    #define QTCOMPAT_KEEP_EMPTY_PARTS Qt::KeepEmptyParts
#endif

// Compatibility with Qt < 5.15
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    #define QTCOMPAT_SPLIT_BEHAVIOR QString::SplitBehavior
#else
    #define QTCOMPAT_SPLIT_BEHAVIOR Qt::SplitBehavior
#endif

// Compatibility with Qt < 5.15
#if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
    #define QTCOMPAT_SKIP_EMPTY_PARTS QString::SkipEmptyParts
#else
    #define QTCOMPAT_SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#endif

// Compatibility with Qt < 5.14
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
    #define QTCOMPAT_DECLARE_RECURSIVE_MUTEX(name) QMutex name { QMutex::Recursive }
#else
    #define QTCOMPAT_DECLARE_RECURSIVE_MUTEX(name) QRecursiveMutex  name
#endif
