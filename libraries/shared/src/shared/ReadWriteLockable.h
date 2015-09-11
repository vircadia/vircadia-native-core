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
#include <QtCore/QReadWriteLock>

class ReadWriteLockable {
public:
    template <typename F>
    bool withWriteLock(F f, bool require = true) {
        if (!require) {
            bool result = _lock.tryLockForWrite();
            if (result) {
                f();
                _lock.unlock();
            }
            return result;
        } 

        QWriteLocker locker(&_lock);
        f();
        return true;
    }

    template <typename F>
    bool withTryWriteLock(F f) {
        return withWriteLock(f, false);
    }

    template <typename F>
    bool withReadLock(F f, bool require = true) const {
        if (!require) {
            bool result = _lock.tryLockForRead();
            if (result) {
                f();
                _lock.unlock();
            }
            return result;
        }

        QReadLocker locker(&_lock);
        f();
        return true;
    }

    template <typename F>
    bool withTryReadLock(F f) const {
        return withReadLock(f, false);
    }

private:
    mutable QReadWriteLock _lock{ QReadWriteLock::Recursive };
};

#endif
