//
//  GenericThread.h
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Generic Threaded or non-threaded processing class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GenericThread_h
#define hifi_GenericThread_h

#include <QtCore/QObject>
#include <QMutex>
#include <QThread>

/// A basic generic "thread" class. Handles a single thread of control within the application. Can operate in non-threaded
/// mode but caller must regularly call threadRoutine() method.
class GenericThread : public QObject {
    Q_OBJECT
public:
    GenericThread();
    virtual ~GenericThread();

    /// Call to start the thread.
    /// \param bool isThreaded true by default. false for non-threaded mode and caller must call threadRoutine() regularly.
    void initialize(bool isThreaded = true, QThread::Priority priority = QThread::NormalPriority);

    /// Call to stop the thread
    void terminate();

    virtual void terminating() { }; // lets your subclass know we're terminating, and it should respond appropriately

    bool isThreaded() const { return _isThreaded; }

    /// Override this function to do whatever your class actually does, return false to exit thread early.
    virtual bool process() = 0;
    virtual void setup() {};
    virtual void shutdown() {};

public slots:
    /// If you're running in non-threaded mode, you must call this regularly
    void threadRoutine();

signals:
    void finished();

protected:

    /// Locks all the resources of the thread.
    void lock() { _mutex.lock(); }

    /// Unlocks all the resources of the thread.
    void unlock() { _mutex.unlock(); }

    bool isStillRunning() const { return !_stopThread; }

protected:
    QMutex _mutex;

    bool _stopThread;
    bool _isThreaded;
    QThread* _thread;
};

#endif // hifi_GenericThread_h
