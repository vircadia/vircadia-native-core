//
//  GenericThread.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Generic Threaded or non-threaded processing class.
//

#ifndef __shared__GenericThread__
#define __shared__GenericThread__

#include <pthread.h>

class GenericThread {
public:
    GenericThread();
    virtual ~GenericThread();

    // Call to start the thread
    void initialize(bool isThreaded);

    // override this function to do whatever your class actually does, return false to exit thread early
    virtual bool process() = 0;

    // Call when you're ready to stop the thread
    void terminate();
    
    // If you're running in non-threaded mode, you must call this regularly
    void* threadRoutine();
    
private:
    bool        _stopThread;
    bool        _isThreaded;
    pthread_t   _thread;
};

extern "C" void* GenericThreadEntry(void* arg);

#endif // __shared__GenericThread__
