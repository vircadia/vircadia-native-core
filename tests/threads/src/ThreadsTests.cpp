//
//  ThreadsTests.cpp
//  thread-tests
//
//  Created by Brad Hefta-Gaub on 2014.03.17
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <QDebug>
#include <QReadWriteLock>

#include <GenericThread.h>
#include <SharedUtil.h>

#include "SampleReadThread.h"
#include "SampleWriteThread.h"
#include "SharedResource.h"
#include "ThreadsTests.h"


void ThreadsTests::runAllTests() {
    qDebug() << "START ThreadsTests::runAllTests()";
    
    const quint64 TOTAL_RUN_TIME = USECS_PER_SECOND * 10; // run for 2 minutes
    const quint64 MAIN_THREAD_SLEEP = USECS_PER_SECOND * 0.25; // main thread checks in every 1/4 second
    quint64 start = usecTimestampNow();
    quint64 now = usecTimestampNow();
    
    SharedResource resource;
    
    const int NUMBER_OF_READERS = 125; // + 3 = 128, Max number of QThreads on the mac.
    QVector<SampleReadThread*> readThreads;
    for(int i = 0; i < NUMBER_OF_READERS; i++) {
        SampleReadThread* newReader = new SampleReadThread(&resource, i);
        newReader->initialize();
        readThreads.append(newReader);
    }

    SampleWriteThread write(&resource);
    write.initialize();

    while(now - start < TOTAL_RUN_TIME) {
        float elapsed = (float)(now - start) / (float)USECS_PER_SECOND;
        qDebug() << "Main thread still running... elapsed:" << elapsed << "seconds";
        qDebug() << "    average Read Lock:" << resource.getAverageLockForReadTime() << "usecs";
        qDebug() << "    average Write Lock:" << resource.getAverageLockForWriteTime() << "usecs";
        qDebug() << "    average Lock:" << resource.getAverageLockTime() << "usecs";
        qDebug() << "    resource.value:" << resource.getValue();
        

        usleep(MAIN_THREAD_SLEEP);
        now = usecTimestampNow();
    }
    write.terminate();
    
    foreach(SampleReadThread* readThread, readThreads) {
        readThread->terminate();
        delete readThread;
    }
    readThreads.clear();

    qDebug() << "DONE running...";
    qDebug() << "    average Read Lock:" << resource.getAverageLockForReadTime() << "usecs";
    qDebug() << "    average Write Lock:" << resource.getAverageLockForWriteTime() << "usecs";
    qDebug() << "    average Lock:" << resource.getAverageLockTime() << "usecs";
    qDebug() << "    resource.value:" << resource.getValue();
    
    qDebug() << "END ThreadsTests::runAllTests()";

    qDebug() << "_POSIX_THREAD_THREADS_MAX" << _POSIX_THREAD_THREADS_MAX;
    qDebug() << "QThread::idealThreadCount()" << QThread::idealThreadCount();

}
