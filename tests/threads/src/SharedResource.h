//
//  SharedResource.h
//  tests
//
//  Created by Brad Hefta-Gaub on 2014.03.17
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__SharedResource__
#define __interface__SharedResource__

#include <QReadWriteLock>

#include <SimpleMovingAverage.h>

/// Generalized threaded processor for handling received inbound packets. 
class SharedResource {
public:
    SharedResource();
    
    void lockForRead();
    void lockForWrite();
    void unlock() { _lock.unlock(); }

    int getValue() const { return _value; }
    int incrementValue() { return ++_value; }
    
    float getAverageLockTime() const { return _lockAverage.getAverage(); }
    float getAverageLockForReadTime() const { return _lockForReadAverage.getAverage(); }
    float getAverageLockForWriteTime() const { return _lockForWriteAverage.getAverage(); }
    
private:
    QReadWriteLock _lock;
    int _value;
    SimpleMovingAverage _lockAverage;
    SimpleMovingAverage _lockForReadAverage;
    SimpleMovingAverage _lockForWriteAverage;
};

#endif // __interface__SharedResource__
