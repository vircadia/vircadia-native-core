//
//  TryLocker.h
//  libraries/shared/src
//
//  Created by Brad Davis on 2015/03/16.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TryLocker_h
#define hifi_TryLocker_h

#include <QMutex>

class MutexTryLocker {
    QMutex& _mutex;
    bool _locked{ false };
public:
    MutexTryLocker(QMutex &m) : _mutex(m), _locked(m.tryLock()) {}
    ~MutexTryLocker() { if (_locked) _mutex.unlock(); }
    bool isLocked() {
        return _locked;
    }
};

#endif // hifi_TryLocker_h
