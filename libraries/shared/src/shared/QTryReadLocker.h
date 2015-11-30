//
//  QTryReadLocker.h
//  shared/src/shared/QTryReadLocker.h
//
//  Created by Clément Brisset on 10/29/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QTryReadLocker_h
#define hifi_QTryReadLocker_h

#include <QtCore/QReadWriteLock>

class QTryReadLocker {
public:
    QTryReadLocker(QReadWriteLock* readWriteLock);
    QTryReadLocker(QReadWriteLock* readWriteLock, int timeout);
    ~QTryReadLocker();
    
    bool isLocked() const;
    
    void unlock();
    bool tryRelock();
    bool tryRelock(int timeout);
    
    QReadWriteLock* readWriteLock() const;
    
private:
    Q_DISABLE_COPY(QTryReadLocker)
    quintptr _val;
};

// Implementation
inline QTryReadLocker::QTryReadLocker(QReadWriteLock* areadWriteLock) :
    _val(reinterpret_cast<quintptr>(areadWriteLock))
{
    Q_ASSERT_X((_val & quintptr(1u)) == quintptr(0),
               "QTryReadLocker", "QTryReadLocker pointer is misaligned");
    tryRelock();
}

inline QTryReadLocker::QTryReadLocker(QReadWriteLock* areadWriteLock, int timeout) :
    _val(reinterpret_cast<quintptr>(areadWriteLock))
{
    Q_ASSERT_X((_val & quintptr(1u)) == quintptr(0),
               "QTryReadLocker", "QTryReadLocker pointer is misaligned");
    tryRelock(timeout);
}

inline QTryReadLocker::~QTryReadLocker() {
    unlock();
}

inline bool QTryReadLocker::isLocked() const {
    return (_val & quintptr(1u)) == quintptr(1u);
}

inline void QTryReadLocker::unlock() {
    if (_val && isLocked()) {
        _val &= ~quintptr(1u);
        readWriteLock()->unlock();
    }
}

inline bool QTryReadLocker::tryRelock() {
    if (_val && !isLocked()) {
        if (readWriteLock()->tryLockForRead()) {
            _val |= quintptr(1u);
            return true;
        }
    }
    return false;
}

inline bool QTryReadLocker::tryRelock(int timeout) {
    if (_val && !isLocked()) {
        if (readWriteLock()->tryLockForRead(timeout)) {
            _val |= quintptr(1u);
            return true;
        }
    }
    return false;
}

inline QReadWriteLock* QTryReadLocker::readWriteLock() const {
    return reinterpret_cast<QReadWriteLock*>(_val & ~quintptr(1u));
}

#endif // hifi_QTryReadLocker_h