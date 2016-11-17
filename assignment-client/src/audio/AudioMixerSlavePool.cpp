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

AudioMixerSlavePool::~AudioMixerSlavePool() {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        wait(lock);
    }
    setNumThreads(0);
}

void AudioMixerSlavePool::mix(const std::vector<SharedNodePointer>& nodes, unsigned int frame) {
    std::unique_lock<std::mutex> lock(_mutex);
    start(lock, nodes, frame);
    wait(lock);
}

void AudioMixerSlavePool::each(std::function<void(AudioMixerSlave& slave)> functor) {
    std::unique_lock<std::mutex> lock(_mutex);
    assert(!_running);

    for (auto& slave : _slaves) {
        functor(*slave.get());
    }
}

void AudioMixerSlavePool::start(std::unique_lock<std::mutex>& lock, const std::vector<SharedNodePointer>& nodes, unsigned int frame) {
    assert(!_running);

    // fill the queue
    for (auto& node : nodes) {
        _queue.emplace(node);
    }

    // toggle running state
    _frame = frame;
    _running = true;
    _numStarted = 0;
    _numFinished = 0;

    _slaveCondition.notify_all();
}

void AudioMixerSlavePool::wait(std::unique_lock<std::mutex>& lock) {
    if (_running) {
        _poolCondition.wait(lock, [&] {
            return _numFinished == _numThreads;
        });
    }

    assert(_queue.empty());

    // toggle running state
    _running = false;
}

void AudioMixerSlavePool::slaveWait() {
    std::unique_lock<std::mutex> lock(_mutex);

    _slaveCondition.wait(lock, [&] {
            return _numStarted != _numThreads;
    });

    // toggle running state
    ++_numStarted;
}

void AudioMixerSlavePool::slaveNotify() {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        ++_numFinished;
    }
    _poolCondition.notify_one();
}

void AudioMixerSlavePool::setNumThreads(int numThreads) {
    std::unique_lock<std::mutex> lock(_mutex);

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
        start(lock, std::vector<SharedNodePointer>(), 0);
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

void AudioMixerSlaveThread::run() {
    while (!_stop) {
        _pool.slaveWait();
        SharedNodePointer node;
        while (_pool._queue.try_pop(node)) {
            mix(node, _pool._frame);
        }
        _pool.slaveNotify();
    }
}
