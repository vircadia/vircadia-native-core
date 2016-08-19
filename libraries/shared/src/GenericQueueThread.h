//
//  Created by Bradley Austin Davis 2015/07/08.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_GenericQueueThread_h
#define hifi_GenericQueueThread_h

#include <stdint.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/QMutex>
#include <QtCore/QQueue>
#include <QtCore/QWaitCondition>

#include "GenericThread.h"
#include "NumericalConstants.h"

template <typename T>
class GenericQueueThread : public GenericThread {
public:
    using Queue = QQueue<T>;
    GenericQueueThread(QObject* parent = nullptr) 
        : GenericThread() {}

    virtual ~GenericQueueThread() {}

    void queueItem(const T& t) {
        lock();
        queueItemInternal(t);
        unlock();
        _hasItems.wakeAll();
    }

    void waitIdle(uint32_t maxWaitMs = UINT32_MAX) {
        QElapsedTimer timer;
        timer.start();

        // FIXME this will work as long as the thread doing the wait
        // is the only thread which can add work to the queue.  
        // It would be better if instead we had a queue empty condition to wait on
        // that would ensure that we get woken as soon as we're idle the very 
        // first time the queue was empty.
        while (timer.elapsed() < maxWaitMs) {
            lock();
            if (!_items.size()) {
                unlock();
                return;
            }
            unlock();
        }
    }

    virtual bool process() {
        lock();
        if (!_items.size()) {
            unlock();
            _hasItemsMutex.lock();
            _hasItems.wait(&_hasItemsMutex, getMaxWait());
            _hasItemsMutex.unlock();
        } else {
            unlock();
        }

        lock();
        if (!_items.size()) {
            unlock();
            return isStillRunning();
        }

        Queue processItems;
        processItems.swap(_items);
        unlock();
        return processQueueItems(processItems);
    }

protected:
    virtual void queueItemInternal(const T& t) {
        _items.push_back(t);
    }

    virtual uint32_t getMaxWait() {
        return MSECS_PER_SECOND;
    }



    virtual bool processQueueItems(const Queue& items) = 0;

    Queue _items;
    QWaitCondition _hasItems;
    QMutex _hasItemsMutex;
};

#endif // hifi_GenericQueueThread_h
