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
    assert(_data.numStarted < _data.numThreads);
    _data.numStarted++;

    bool starting = true;
    while (true) {
        bool stopping = _stop;
        notify(stopping);
        if (stopping) {
            return;
        }

        wait(starting);
        starting = false;
        assert(_function);

        if (_function) {
            // iterate over all available nodes
            SharedNodePointer node;
            while (try_pop(node)) {
                (this->*_function)(node);
            }
        }
    }
}

void AvatarMixerWorkerThread::wait(bool starting) {
    assert(_data.numStarted <= _data.numThreads);

    {
        Lock workerLock(_data.workerMutex);
        if (starting || _data.numStarted == _data.numThreads) {
            _data.workerCondition.wait(workerLock, [&] {
                assert(_data.numStarted <= _data.numThreads);
                return _data.numStarted != _data.numThreads;
            });
        }
        _data.numStarted++;
    }

    if (_data.configure) {
        _data.configure(*this);
    }
    _function = _data.function;
    assert(_function);
}

void AvatarMixerWorkerThread::notify(bool stopping) {
    assert(_data.numFinished < _data.numThreads);
    assert(_data.numFinished <= _data.numStarted);
    int numFinished = ++_data.numFinished;
    if (stopping) {
        _data.numStopped++;
        assert(_data.numStopped <= _data.numFinished);
    }

    if (numFinished == _data.numThreads) {
        Lock poolLock(_data.poolMutex);
        _data.poolCondition.notify_one();
    }
}

bool AvatarMixerWorkerThread::try_pop(SharedNodePointer& node) {
    return _data.queue.try_pop(node);
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
    {
        Lock workerLock(_data.workerMutex);
        _data.numStarted = _data.numFinished = 0;
        _data.workerCondition.notify_all();
    }

    // wait
    {
        Lock poolLock(_data.poolMutex);
        if (_data.numFinished < _data.numThreads) {
            _data.poolCondition.wait(poolLock, [&] {
                assert(_data.numFinished <= _data.numThreads);
                return _data.numFinished == _data.numThreads;
            });
        }
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
        assert(_data.numFinished == _data.numThreads);

        // start new slaves
        {
            Lock workerLock(_data.workerMutex);
            while (numThreads > _data.numThreads) {
                _data.numThreads++;
                auto worker = new AvatarMixerWorkerThread(_data, _slaveSharedData);
                worker->start();
                _workers.emplace_back(worker);
            }
        }

        // wait for the new workers to wake up and enter the wait
        Lock poolLock(_data.poolMutex);
        if (_data.numFinished < _data.numThreads) {
            _data.poolCondition.wait(poolLock, [&] {
                assert(_data.numFinished <= _data.numThreads);
                return _data.numFinished == _data.numThreads;
            });
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
            {
                Lock workerLock(_data.workerMutex);
                _data.numStarted = _data.numFinished = 0;
                _data.workerCondition.notify_all();
            }
            
            {
                Lock poolLock(_data.poolMutex);
                if (_data.numFinished < _data.numThreads) {
                    _data.poolCondition.wait(poolLock, [&] {
                        assert(_data.numFinished <= _data.numThreads);
                        return _data.numFinished == _data.numThreads;
                    });
                }
            }
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
    qDebug("%s: completed", __FUNCTION__);
}
