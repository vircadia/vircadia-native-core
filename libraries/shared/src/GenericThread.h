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

/// A basic generic "thread" class. Handles a single thread of control within the application. Can operate in non-threaded
/// mode but caller must regularly call threadRoutine() method.
class GenericThread {
public:
    GenericThread();
    virtual ~GenericThread();

    /// Call to start the thread. 
    /// \param bool isThreaded true by default. false for non-threaded mode and caller must call threadRoutine() regularly.
    void initialize(bool isThreaded = true);

    /// Call to stop the thread
    void terminate();
    
    /// If you're running in non-threaded mode, you must call this regularly
    void* threadRoutine();

    /// Override this function to do whatever your class actually does, return false to exit thread early.
    virtual bool process() = 0;

    bool isThreaded() const { return _isThreaded; }

protected:

    /// Locks all the resources of the thread.
    void lock() { pthread_mutex_lock(&_mutex); }

    /// Unlocks all the resources of the thread.
    void unlock() { pthread_mutex_unlock(&_mutex); }
    
    bool isStillRunning() const { return !_stopThread; }

private:
    pthread_mutex_t _mutex;

    bool _stopThread;
    bool _isThreaded;
    pthread_t _thread;
};

extern "C" void* GenericThreadEntry(void* arg);

#endif // __shared__GenericThread__
