//
//  SampleReadThread.cpp
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
#include "SampleReadThread.h"

SampleReadThread::SampleReadThread(SharedResource* theResource, int id) :
    _theResource(theResource),
    _myID(id) {
}

bool SampleReadThread::process() {
    const quint64 FRAME_RATE = 60;
    const quint64 USECS_PER_FRAME = USECS_PER_SECOND / FRAME_RATE; // every 60fps

    quint64 start = usecTimestampNow();
    _theResource->lockForRead();
    quint64 end = usecTimestampNow();
    quint64 elapsed = end - start;

    int theValue = _theResource->getValue();

    bool wantDebugging = true;
    if (wantDebugging) {
        qDebug() << "SampleReadThread::process()... _myID=" << _myID << "lockForRead() took" << elapsed << "usecs" <<
            " _theResource->getValue()=" << theValue;
    }
    
    quint64 startWork = usecTimestampNow();
    {
        const int LOTS_OF_OPERATIONS = 100000;
        for(int i = 0; i < LOTS_OF_OPERATIONS; i++) {
            float x = rand();
            float y = rand();
            float z = x * y;
        }
    }
    quint64 endWork = usecTimestampNow();
    quint64 elapsedWork = endWork - startWork;
    qDebug() << "SampleReadThread::process()... _myID=" << _myID << " work took" << elapsedWork << "usecs";

    _theResource->unlock();


    if (isStillRunning()) {
        if (elapsed < USECS_PER_FRAME) {
            quint64 sleepFor = USECS_PER_FRAME - elapsed;
            usleep(sleepFor);
        } else {
            usleep(5); // sleep at least a little
        }
    }
    return isStillRunning();  // keep running till they terminate us
}
