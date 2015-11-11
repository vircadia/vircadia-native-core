//
//  QTryWriteLocker.h
//  shared/src/shared/QTryWriteLocker.h
//
//  Created by Clément Brisset on 10/29/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QTryWriteLocker_h
#define hifi_QTryWriteLocker_h

#include <QtCore/QReadWriteLock>

class QTryWriteLocker {
public:
    QTryWriteLocker(QReadWriteLock* readWriteLock);
    QTryWriteLocker(QReadWriteLock* readWriteLock, int timeout);
    ~QTryWriteLocker();
    
    bool isLocked() const;
    
    void unlock();
    bool tryRelock();
    bool tryRelock(int timeout);
    
    QReadWriteLock* readWriteLock() const;
    
private:
    Q_DISABLE_COPY(QTryWriteLocker)
    quintptr q_val;
};

// Implementation
inline QTryWriteLocker::QTryWriteLocker(QReadWriteLock *readWriteLock) :
    q_val(reinterpret_cast<quintptr>(readWriteLock))
{
    Q_ASSERT_X((q_val & quintptr(1u)) == quintptr(0),
               "QTryWriteLocker", "QTryWriteLocker pointer is misaligned");
    tryRelock();
}

inline QTryWriteLocker::QTryWriteLocker(QReadWriteLock *readWriteLock, int timeout) :
    q_val(reinterpret_cast<quintptr>(readWriteLock))
{
    Q_ASSERT_X((q_val & quintptr(1u)) == quintptr(0),
               "QTryWriteLocker", "QTryWriteLocker pointer is misaligned");
    tryRelock(timeout);
}

inline QTryWriteLocker::~QTryWriteLocker() {
    unlock();
}

inline bool QTryWriteLocker::isLocked() const {
    return (q_val & quintptr(1u)) == quintptr(1u);
}

inline void QTryWriteLocker::unlock() {
    if (q_val && isLocked()) {
        q_val &= ~quintptr(1u);
        readWriteLock()->unlock();
    }
}

inline bool QTryWriteLocker::tryRelock() {
    if (q_val && !isLocked()) {
        if (readWriteLock()->tryLockForWrite()) {
            q_val |= quintptr(1u);
            return true;
        }
    }
    return false;
}

inline bool QTryWriteLocker::tryRelock(int timeout) {
    if (q_val && !isLocked()) {
        if (readWriteLock()->tryLockForWrite(timeout)) {
            q_val |= quintptr(1u);
            return true;
        }
    }
    return false;
}

inline QReadWriteLock* QTryWriteLocker::readWriteLock() const {
    return reinterpret_cast<QReadWriteLock*>(q_val & ~quintptr(1u));
}

#endif // hifi_QTryWriteLocker_h