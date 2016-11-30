//
//  AudioMixerSlavePool.cpp
//  assignment-client/src/audio
//
//  Created by Zach Pomerantz on 11/16/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <algorithm>

#include "AudioMixerSlavePool.h"

void AudioMixerSlaveThread::run() {
    while (!_stop) {
        wait();

        SharedNodePointer node;
        while (try_pop(node)) {
            mix(node);
        }

        notify();
    }
}

void AudioMixerSlaveThread::wait() {
    Lock lock(_pool._mutex);

    _pool._slaveCondition.wait(lock, [&] {
         return _pool._numStarted != _pool._numThreads;
    });

    // toggle running state
    ++_pool._numStarted;
    configure(_pool._begin, _pool._end, _pool._frame);
}

void AudioMixerSlaveThread::notify() {
    {
        Lock lock(_pool._mutex);
        ++_pool._numFinished;
    }
    _pool._poolCondition.notify_one();
}

bool AudioMixerSlaveThread::try_pop(SharedNodePointer& node) {
    return _pool._queue.try_pop(node);
}

AudioMixerSlavePool::~AudioMixerSlavePool() {
    {
        Lock lock(_mutex);
        wait(lock);
    }
    setNumThreads(0);
}

void AudioMixerSlavePool::mix(ConstIter begin, ConstIter end, unsigned int frame) {
    Lock lock(_mutex);
    start(lock, begin, end, frame);
    wait(lock);
}

void AudioMixerSlavePool::each(std::function<void(AudioMixerSlave& slave)> functor) {
    Lock lock(_mutex);
    assert(!_running);

    for (auto& slave : _slaves) {
        functor(*slave.get());
    }
}

void AudioMixerSlavePool::start(Lock& lock, ConstIter begin, ConstIter end, unsigned int frame) {
    assert(!_running);

    // fill the queue
    std::for_each(begin, end, [&](const SharedNodePointer& node) {
        _queue.emplace(node);
    });

    // toggle running state
    _frame = frame;
    _running = true;
    _numStarted = 0;
    _numFinished = 0;
    _begin = begin;
    _end = end;

    _slaveCondition.notify_all();
}

void AudioMixerSlavePool::wait(Lock& lock) {
    if (_running) {
        _poolCondition.wait(lock, [&] {
            return _numFinished == _numThreads;
        });
    }

    assert(_queue.empty());

    // toggle running state
    _running = false;
}

void AudioMixerSlavePool::setNumThreads(int numThreads) {
    Lock lock(_mutex);

    // ensure slave are not running
    assert(!_running);

    // clamp to allowed size
    {
        // idealThreadCount returns -1 if cores cannot be detected - cast it to a large number
        int maxThreads = (int)((unsigned int)QThread::idealThreadCount());
        int clampedThreads = std::min(std::max(1, numThreads), maxThreads);
        if (clampedThreads != numThreads) {
            qWarning("%s: clamped to %d (was %d)", __FUNCTION__, numThreads, clampedThreads);
            numThreads = clampedThreads;
        }
    }
    qDebug("%s: set %d threads", __FUNCTION__, numThreads);

    if (numThreads > _numThreads) {
        // start new slaves
        for (int i = 0; i < numThreads - _numThreads; ++i) {
            auto slave = new AudioMixerSlaveThread(*this);
            slave->start();
            _slaves.emplace_back(slave);
        }
    } else if (numThreads < _numThreads) {
        auto extraBegin = _slaves.begin() + _numThreads;

        // stop extra slaves...
        auto slave = extraBegin;
        while (slave != _slaves.end()) {
            (*slave)->stop();
        }

        // ...cycle slaves with empty queue...
        start(lock);
        wait(lock);

        // ...wait for them to finish...
        slave = extraBegin;
        while (slave != _slaves.end()) {
            (*slave)->wait();
        }

        // ...and delete them
        _slaves.erase(extraBegin, _slaves.end());
    }

    _numThreads = _numStarted = _numFinished = numThreads;
}
