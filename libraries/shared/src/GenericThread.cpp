//
//  GenericThread.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Generic Threaded or non-threaded processing class
//

#include "GenericThread.h"

GenericThread::GenericThread() :
    _stopThread(false),
    _isThreaded(false) // assume non-threaded, must call initialize()
{
}

GenericThread::~GenericThread() {
    terminate();
}

void GenericThread::initialize(bool isThreaded) {
    _isThreaded = isThreaded;
    if (_isThreaded) {
        QThread* _thread = new QThread(this);

        // when the worker thread is started, call our engine's run..
        connect(_thread, SIGNAL(started()), this, SLOT(threadRoutine()));

        // XXXBHG: this is a known memory leak/thread leak. I will fix this shortly.
        //connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
        //connect(_thread, SIGNAL(finished()), _thread, SLOT(deleteLater()));

        this->moveToThread(_thread);

        // Starts an event loop, and emits _thread->started()
        _thread->start();
    }
}

void GenericThread::terminate() {
    if (_isThreaded) {
        _stopThread = true;
        //_isThreaded = false;
    }
}

void GenericThread::threadRoutine() {
    while (!_stopThread) {

        // override this function to do whatever your class actually does, return false to exit thread early
        if (!process()) {
            break;
        }

        // In non-threaded mode, this will break each time you call it so it's the
        // callers responsibility to continuously call this method
        if (!_isThreaded) {
            break;
        }
    }

    if (_isThreaded) {
        emit finished();
    }
}
