//
//  AvatarMixerSlavePool.cpp
//  assignment-client/src/avatar
//
//  Created by Brad Hefta-Gaub on 2/14/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarMixerSlavePool.h"

#include <assert.h>
#include <algorithm>

void AvatarMixerWorkerThread::run() {
    while (true) {
        wait();

        // iterate over all available nodes
        SharedNodePointer node;
        while (try_pop(node)) {
            (this->*_function)(node);
        }

        bool stopping = _stop;
        notify(stopping);
        if (stopping) {
            return;
        }
    }
}

void AvatarMixerWorkerThread::wait() {
    {
        Lock workerLock(_pool.workerMutex);
        _pool.workerCondition.wait(workerLock, [&] {
            assert(_pool.numStarted <= _pool.numThreads);
            return _pool.numStarted != _pool.numThreads;
        });
    }
    ++_pool.numStarted;

    if (_pool.configure) {
        _pool.configure(*this);
    }
    _function = _pool.function;
}

void AvatarMixerWorkerThread::notify(bool stopping) {
    assert(_pool.numFinished < _pool.numThreads && _pool.numFinished <= _pool.numStarted);
    int numFinished = ++_pool.numFinished;
    if (stopping) {
        ++_pool.numStopped;
        assert(_pool.numStopped <= _pool.numFinished);
    }

    if (numFinished == _pool.numThreads) {
        _pool.poolCondition.notify_one();
    }
}

bool AvatarMixerWorkerThread::try_pop(SharedNodePointer& node) {
    return _pool.queue.try_pop(node);
}

void AvatarMixerSlavePool::processIncomingPackets(ConstIter begin, ConstIter end) {
    _data.function = &AvatarMixerSlave::processIncomingPackets;
    _data.configure = [=](AvatarMixerSlave& slave) {
        slave.configure(begin, end);
    };
    run(begin, end);
}

void AvatarMixerSlavePool::broadcastAvatarData(ConstIter begin, ConstIter end, 
                                               p_high_resolution_clock::time_point lastFrameTimestamp,
                                               float maxKbpsPerNode, float throttlingRatio) {
    _data.function = &AvatarMixerSlave::broadcastAvatarData;
    _data.configure = [=](AvatarMixerSlave& slave) {
        slave.configureBroadcast(begin, end, lastFrameTimestamp, maxKbpsPerNode, throttlingRatio, _priorityReservedFraction);
    };
    run(begin, end);
}

void AvatarMixerSlavePool::run(ConstIter begin, ConstIter end) {
    _begin = begin;
    _end = end;

    // fill the queue
    std::for_each(_begin, _end, [&](const SharedNodePointer& node) {
        _data.queue.push(node);
    });

    // run
    _data.numStarted = _data.numFinished = 0;
    _data.workerCondition.notify_all();

    // wait
    {
        Lock poolLock(_data.poolMutex);
        _data.poolCondition.wait(poolLock, [&] {
            assert(_data.numFinished <= _data.numThreads);
            return _data.numFinished == _data.numThreads;
        });
    }
    assert(_data.numStarted == _data.numThreads);

    assert(_data.queue.empty());
}


void AvatarMixerSlavePool::each(std::function<void(AvatarMixerSlave& slave)> functor) {
    for (auto& worker : _workers) {
        functor(*worker.get());
    }
}

#ifdef DEBUG_EVENT_QUEUE
void AvatarMixerSlavePool::queueStats(QJsonObject& stats) {
    unsigned i = 0;
    for (auto& worker : _workers) {
        int queueSize = ::hifi::qt::getEventQueueSize(worker.get());
        QString queueName = QString("avatar_thread_event_queue_%1").arg(i);
        stats[queueName] = queueSize;

        i++;
    }
}
#endif // DEBUG_EVENT_QUEUE

void AvatarMixerSlavePool::setNumThreads(int numThreads) {
    // clamp to allowed size
    {
        int maxThreads = QThread::idealThreadCount();
        if (maxThreads == -1) {
            // idealThreadCount returns -1 if cores cannot be detected
            static const int MAX_THREADS_IF_UNKNOWN = 4;
            maxThreads = MAX_THREADS_IF_UNKNOWN;
        }

        int clampedThreads = std::min(std::max(1, numThreads), maxThreads);
        if (clampedThreads != numThreads) {
            qWarning("%s: clamped to %d (was %d)", __FUNCTION__, clampedThreads, numThreads);
            numThreads = clampedThreads;
        }
    }

    resize(numThreads);
}

void AvatarMixerSlavePool::resize(int numThreads) {
    assert(_data.numThreads == (int)_workers.size());

    qDebug("%s: set %d threads (was %d)", __FUNCTION__, numThreads, _data.numThreads);

    if (numThreads > _data.numThreads) {
        // start new slaves
        for (int i = 0; i < numThreads - _data.numThreads; ++i) {
            auto worker = new AvatarMixerWorkerThread(_data, _slaveSharedData);
            worker->start();
            _workers.emplace_back(worker);
        }
    } else if (numThreads < _data.numThreads) {
        auto extraBegin = _workers.begin() + numThreads;

        // mark slaves to stop...
        auto worker = extraBegin;
        while (worker != _workers.end()) {
            (*worker)->stop();
            ++worker;
        }

        // ...cycle them until they do stop...
        _data.numStopped = 0;
        while (_data.numStopped != (_data.numThreads - numThreads)) {
            _data.numStarted = _data.numFinished = 0;
            _data.workerCondition.notify_all();
            
            Lock poolLock(_data.poolMutex);
            _data.poolCondition.wait(poolLock, [&] {
                assert(_data.numFinished <= _data.numThreads);
                return _data.numFinished == _data.numThreads;
            });
            assert(_data.numStopped == (_data.numThreads - numThreads));
        }

        // ...wait for threads to finish...
        worker = extraBegin;
        while (worker != _workers.end()) {
            QThread* thread = reinterpret_cast<QThread*>(worker->get());
            static const int MAX_THREAD_WAIT_TIME = 10;
            thread->wait(MAX_THREAD_WAIT_TIME);
            ++worker;
        }

        // ...and erase them
        _workers.erase(extraBegin, _workers.end());
    }

    _data.numThreads = _data.numStarted = _data.numFinished = numThreads;
    assert(_data.numThreads == (int)_workers.size());
}
