//
//  GenericThread.cpp
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>

#include "GenericThread.h"


GenericThread::GenericThread() :
    _stopThread(false),
    _isThreaded(false) // assume non-threaded, must call initialize()
{
    
}

GenericThread::~GenericThread() {
    // we only need to call terminate() if we're actually threaded and still running
    if (isStillRunning() && isThreaded()) {
        terminate();
    }
}

void GenericThread::initialize(bool isThreaded, QThread::Priority priority) {
    _isThreaded = isThreaded;
    if (_isThreaded) {
        _thread = new QThread(this);
        
        // match the thread name to our object name
        _thread->setObjectName(objectName());

        // when the worker thread is started, call our engine's run..
        connect(_thread, &QThread::started, this, &GenericThread::threadRoutine);
        connect(_thread, &QThread::finished, this, &GenericThread::finished);

        moveToThread(_thread);

        // Starts an event loop, and emits _thread->started()
        _thread->start();
        
        _thread->setPriority(priority);
    } else {
        setup();
    }
}

void GenericThread::terminate() {
    if (_isThreaded) {
        _stopThread = true;
        
        terminating();

        if (_thread) {
            _thread->wait();
            _thread->deleteLater();
            _thread = NULL;
        }
    } else {
        shutdown();
    }
}

void GenericThread::threadRoutine() {
    if (_isThreaded) {
        setup();
    }

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
        shutdown();

        // If we were on a thread, then quit our thread
        if (_thread) {
            _thread->quit();
        }
    }
    
}
