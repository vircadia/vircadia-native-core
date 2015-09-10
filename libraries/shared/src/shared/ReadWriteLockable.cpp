//
//  Created by Bradley Austin Davis on 2015/09/10
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ReadWriteLockable.h"

#include <QtCore/QThread>
//
//bool ReadWriteLockable::isLocked() const {
//    bool readSuccess = _lock.tryLockForRead();
//    if (readSuccess) {
//        _lock.unlock();
//    }
//    bool writeSuccess = _lock.tryLockForWrite();
//    if (writeSuccess) {
//        _lock.unlock();
//    }
//    if (readSuccess && writeSuccess) {
//        return false;  // if we can take both kinds of lock, there was no previous lock
//    }
//    return true; // either read or write failed, so there is some lock in place.
//}
//
//
//bool ReadWriteLockable::isWriteLocked() const {
//    bool readSuccess = _lock.tryLockForRead();
//    if (readSuccess) {
//        _lock.unlock();
//        return false;
//    }
//    bool writeSuccess = _lock.tryLockForWrite();
//    if (writeSuccess) {
//        _lock.unlock();
//        return false;
//    }
//    return true; // either read or write failed, so there is some lock in place.
//}
//
//
//bool ReadWriteLockable::isUnlocked() const {
//    // this can't be sure -- this may get unlucky and hit locks from other threads.  what we're actually trying
//    // to discover is if *this* thread hasn't locked the EntityItem.  Try repeatedly to take both kinds of lock.
//    bool readSuccess = false;
//    for (int i = 0; i<80; i++) {
//        readSuccess = _lock.tryLockForRead();
//        if (readSuccess) {
//            _lock.unlock();
//            break;
//        }
//        QThread::usleep(200);
//    }
//
//    bool writeSuccess = false;
//    if (readSuccess) {
//        for (int i = 0; i<80; i++) {
//            writeSuccess = _lock.tryLockForWrite();
//            if (writeSuccess) {
//                _lock.unlock();
//                break;
//            }
//            QThread::usleep(300);
//        }
//    }
//
//    if (readSuccess && writeSuccess) {
//        return true;  // if we can take both kinds of lock, there was no previous lock
//    }
//    return false;
//}
//
//void ReadWriteLockable::lockForRead() const {
//    _lock.lockForRead(); 
//}
//
//bool ReadWriteLockable::tryLockForRead() const {
//    return _lock.tryLockForRead(); 
//}
//
//void ReadWriteLockable::lockForWrite() { 
//    _lock.lockForWrite(); 
//}
//
//bool ReadWriteLockable::tryLockForWrite() { 
//    return _lock.tryLockForWrite(); 
//}
//
//void ReadWriteLockable::unlock() const { 
//    _lock.unlock(); 
//}
