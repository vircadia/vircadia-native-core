//
//  SharedResource.cpp
//  tests
//
//  Created by Brad Hefta-Gaub on 2014.03.17
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <SharedUtil.h>

#include "SharedResource.h"

const int MOVING_AVERAGE_SAMPLES = 10000;

SharedResource::SharedResource() : 
    _value(0),
    _lockAverage(MOVING_AVERAGE_SAMPLES),
    _lockForReadAverage(MOVING_AVERAGE_SAMPLES),
    _lockForWriteAverage(MOVING_AVERAGE_SAMPLES)
{ 
};
    
void SharedResource::lockForRead() { 
    quint64 start = usecTimestampNow();
    _lock.lockForRead(); 
    quint64 end = usecTimestampNow();
    quint64 elapsed = end - start;
    _lockForReadAverage.updateAverage(elapsed);
    _lockAverage.updateAverage(elapsed);
}
void SharedResource::lockForWrite() { 
    quint64 start = usecTimestampNow();
    _lock.lockForWrite(); 
    quint64 end = usecTimestampNow();
    quint64 elapsed = end - start;
    _lockForWriteAverage.updateAverage(elapsed);
    _lockAverage.updateAverage(elapsed);
}
