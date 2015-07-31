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

#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

#include "GenericThread.h"
#include "NumericalConstants.h"

template <typename T>
class GenericQueueThread : public GenericThread {
public:
    using Queue = QQueue<T>;
    GenericQueueThread(QObject* parent = nullptr) 
        : GenericThread(parent) {}

    virtual ~GenericQueueThread() {}

    void queueItem(const T& t) {
        lock();
        queueItemInternal(t);
        unlock();
        _hasItems.wakeAll();
    }

protected:
    virtual void queueItemInternal(const T& t) {
        _items.push_back(t);
    }

    virtual uint32_t getMaxWait() {
        return MSECS_PER_SECOND;
    }

    virtual bool process() {
        if (!_items.size()) {
            _hasItemsMutex.lock();
            _hasItems.wait(&_hasItemsMutex, getMaxWait());
            _hasItemsMutex.unlock();
        }

        if (!_items.size()) {
            return isStillRunning();
        }

        Queue processItems;
        lock();
        processItems.swap(_items);
        unlock();
        return processQueueItems(processItems);
    }

    virtual bool processQueueItems(const Queue& items) = 0;

    Queue _items;
    QWaitCondition _hasItems;
    QMutex _hasItemsMutex;
};

#endif // hifi_GenericQueueThread_h
