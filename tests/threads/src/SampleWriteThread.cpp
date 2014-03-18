//
//  SampleWriteThread.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 2014.03.17
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded interface thread for hiding and showing voxels in the local tree.
//

#include <QDebug>

#include <SharedUtil.h>

#include "SharedResource.h"
#include "SampleWriteThread.h"

SampleWriteThread::SampleWriteThread(SharedResource* theResource) :
    _theResource(theResource) {
}

bool SampleWriteThread::process() {
    const quint64 USECS_PER_OPERATION = USECS_PER_SECOND * 1; // once every second.

    quint64 start = usecTimestampNow();
    _theResource->lockForWrite();
    quint64 end = usecTimestampNow();
    quint64 elapsed = end - start;

    int theValue = _theResource->incrementValue();

    bool wantDebugging = false;
    if (wantDebugging) {
        qDebug() << "SampleWriteThread::process()... lockForWrite() took" << elapsed << "usecs" <<
            " _theResource->incrementValue()=" << theValue;
    }
    _theResource->unlock();


    if (isStillRunning()) {
        if (elapsed < USECS_PER_OPERATION) {
            quint64 sleepFor = USECS_PER_OPERATION - elapsed;
            usleep(sleepFor);
        }
    }
    return isStillRunning();  // keep running till they terminate us
}
