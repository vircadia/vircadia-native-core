//
//  Created by Bradley Austin Davis on 2015/09/10
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_ReadWriteLockable_h
#define hifi_ReadWriteLockable_h

#include <utility>

#include <QtCore/QReadWriteLock>

#include "QTryReadLocker.h"
#include "QTryWriteLocker.h"

class ReadWriteLockable {
public:
    // Write locks
    template <typename F>
    void withWriteLock(F&& f) const;
    
    template <typename F>
    bool withWriteLock(F&& f, bool require) const;

    template <typename F>
    bool withTryWriteLock(F&& f) const;
    
    template <typename F>
    bool withTryWriteLock(F&& f, int timeout) const;
    
    // Read locks
    template <typename F>
    void withReadLock(F&& f) const;
    
    template <typename F>
    bool withReadLock(F&& f, bool require) const;
    
    template <typename F>
    bool withTryReadLock(F&& f) const;
    
    template <typename F>
    bool withTryReadLock(F&& f, int timeout) const;

    QReadWriteLock& getLock() const { return _lock; }

private:
    mutable QReadWriteLock _lock { QReadWriteLock::Recursive };
};

// ReadWriteLockable
template <typename F>
inline void ReadWriteLockable::withWriteLock(F&& f) const {
    QWriteLocker locker(&_lock);
    f();
}

template <typename F>
inline bool ReadWriteLockable::withWriteLock(F&& f, bool require) const {
    if (require) {
        withWriteLock(std::forward<F>(f));
        return true;
    } else {
        return withTryReadLock(std::forward<F>(f));
    }
}

template <typename F>
inline bool ReadWriteLockable::withTryWriteLock(F&& f) const {
    QTryWriteLocker locker(&_lock);
    if (locker.isLocked()) {
        f();
        return true;
    }
    return false;
}

template <typename F>
inline bool ReadWriteLockable::withTryWriteLock(F&& f, int timeout) const {
    QTryWriteLocker locker(&_lock, timeout);
    if (locker.isLocked()) {
        f();
        return true;
    }
    return false;
}

template <typename F>
inline void ReadWriteLockable::withReadLock(F&& f) const {
    QReadLocker locker(&_lock);
    f();
}

template <typename F>
inline bool ReadWriteLockable::withReadLock(F&& f, bool require) const {
    if (require) {
        withReadLock(std::forward<F>(f));
        return true;
    } else {
        return withTryReadLock(std::forward<F>(f));
    }
}

template <typename F>
inline bool ReadWriteLockable::withTryReadLock(F&& f) const {
    QTryReadLocker locker(&_lock);
    if (locker.isLocked()) {
        f();
        return true;
    }
    return false;
}

template <typename F>
inline bool ReadWriteLockable::withTryReadLock(F&& f, int timeout) const {
    QTryReadLocker locker(&_lock, timeout);
    if (locker.isLocked()) {
        f();
        return true;
    }
    return false;
}


#endif
