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

#include <assert.h>
#include <algorithm>

#include "AvatarMixerSlavePool.h"

void AvatarMixerSlaveThread::run() {
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

void AvatarMixerSlaveThread::wait() {
    {
        Lock lock(_pool._mutex);
        _pool._slaveCondition.wait(lock, [&] {
            assert(_pool._numStarted <= _pool._numThreads);
            return _pool._numStarted != _pool._numThreads;
        });
        ++_pool._numStarted;
    }
    if (_pool._configure) {
        _pool._configure(*this);
    }
    _function = _pool._function;
}

void AvatarMixerSlaveThread::notify(bool stopping) {
    {
        Lock lock(_pool._mutex);
        assert(_pool._numFinished < _pool._numThreads);
        ++_pool._numFinished;
        if (stopping) {
            ++_pool._numStopped;
        }
    }
    _pool._poolCondition.notify_one();
}

bool AvatarMixerSlaveThread::try_pop(SharedNodePointer& node) {
    return _pool._queue.try_pop(node);
}

#ifdef AVATAR_SINGLE_THREADED
static AvatarMixerSlave slave;
#endif

void AvatarMixerSlavePool::processIncomingPackets(ConstIter begin, ConstIter end) {
    _function = &AvatarMixerSlave::processIncomingPackets;
    _configure = [=](AvatarMixerSlave& slave) { 
        slave.configure(begin, end);
    };
    run(begin, end);
}

void AvatarMixerSlavePool::broadcastAvatarData(ConstIter begin, ConstIter end, 
                                               p_high_resolution_clock::time_point lastFrameTimestamp,
                                               float maxKbpsPerNode, float throttlingRatio) {
    _function = &AvatarMixerSlave::broadcastAvatarData;
    _configure = [=](AvatarMixerSlave& slave) { 
        slave.configureBroadcast(begin, end, lastFrameTimestamp, maxKbpsPerNode, throttlingRatio);
   };
    run(begin, end);
}

void AvatarMixerSlavePool::run(ConstIter begin, ConstIter end) {
    _begin = begin;
    _end = end;

#ifdef AUDIO_SINGLE_THREADED
    _configure(slave);
    std::for_each(begin, end, [&](const SharedNodePointer& node) {
        _function(slave, node);
});
#else
    // fill the queue
    std::for_each(_begin, _end, [&](const SharedNodePointer& node) {
        _queue.emplace(node);
    });

    {
        Lock lock(_mutex);

        // run
        _numStarted = _numFinished = 0;
        _slaveCondition.notify_all();

        // wait
        _poolCondition.wait(lock, [&] {
            assert(_numFinished <= _numThreads);
            return _numFinished == _numThreads;
        });

        assert(_numStarted == _numThreads);
    }

    assert(_queue.empty());
#endif
}


void AvatarMixerSlavePool::each(std::function<void(AvatarMixerSlave& slave)> functor) {
#ifdef AVATAR_SINGLE_THREADED
    functor(slave);
#else
    for (auto& slave : _slaves) {
        functor(*slave.get());
    }
#endif
}

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
    assert(_numThreads == (int)_slaves.size());

#ifdef AVATAR_SINGLE_THREADED
    qDebug("%s: running single threaded", __FUNCTION__, numThreads);
#else
    qDebug("%s: set %d threads (was %d)", __FUNCTION__, numThreads, _numThreads);

    Lock lock(_mutex);

    if (numThreads > _numThreads) {
        // start new slaves
        for (int i = 0; i < numThreads - _numThreads; ++i) {
            auto slave = new AvatarMixerSlaveThread(*this);
            slave->start();
            _slaves.emplace_back(slave);
        }
    } else if (numThreads < _numThreads) {
        auto extraBegin = _slaves.begin() + numThreads;

        // mark slaves to stop...
        auto slave = extraBegin;
        while (slave != _slaves.end()) {
            (*slave)->_stop = true;
            ++slave;
        }

        // ...cycle them until they do stop...
        _numStopped = 0;
        while (_numStopped != (_numThreads - numThreads)) {
            _numStarted = _numFinished = _numStopped;
            _slaveCondition.notify_all();
            _poolCondition.wait(lock, [&] {
                assert(_numFinished <= _numThreads);
                return _numFinished == _numThreads;
            });
        }

        // ...wait for threads to finish...
        slave = extraBegin;
        while (slave != _slaves.end()) {
            QThread* thread = reinterpret_cast<QThread*>(slave->get());
            static const int MAX_THREAD_WAIT_TIME = 10;
            thread->wait(MAX_THREAD_WAIT_TIME);
            ++slave;
        }

        // ...and erase them
        _slaves.erase(extraBegin, _slaves.end());
    }

    _numThreads = _numStarted = _numFinished = numThreads;
    assert(_numThreads == (int)_slaves.size());
#endif
}
