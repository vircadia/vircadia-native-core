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
    pthread_mutex_init(&_mutex, 0);
}

GenericThread::~GenericThread() {
    terminate();
    pthread_mutex_destroy(&_mutex);
}

void GenericThread::initialize(bool isThreaded) {
    _isThreaded = isThreaded;
    if (_isThreaded) {
        pthread_create(&_thread, NULL, GenericThreadEntry, this);
    }
}

void GenericThread::terminate() {
    if (_isThreaded) {
        _stopThread = true;
        pthread_join(_thread, NULL); 
        _isThreaded = false;
    }
}

void* GenericThread::threadRoutine() {
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
        pthread_exit(0); 
    }
    return NULL; 
}

extern "C" void* GenericThreadEntry(void* arg) {
    GenericThread* genericThread = (GenericThread*)arg;
    return genericThread->threadRoutine();
}