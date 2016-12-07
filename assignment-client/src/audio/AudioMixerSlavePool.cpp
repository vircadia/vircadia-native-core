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
        wait(); // for the audio pool to notify

        // iterate over all available nodes
        SharedNodePointer node;
        while (try_pop(node)) {
            mix(node);
        }

        notify(); // the audio pool we are done
    }
}

void AudioMixerSlaveThread::wait() {
    {
        Lock lock(_pool._mutex);
        _pool._slaveCondition.wait(lock, [&] {
            assert(_pool._numStarted <= _pool._numThreads);
            return _pool._numStarted != _pool._numThreads;
        });
        ++_pool._numStarted;
    }
    configure(_pool._begin, _pool._end, _pool._frame);
}

void AudioMixerSlaveThread::notify() {
    {
        Lock lock(_pool._mutex);
        assert(_pool._numFinished < _pool._numThreads);
        ++_pool._numFinished;
    }
    _pool._poolCondition.notify_one();
}

bool AudioMixerSlaveThread::try_pop(SharedNodePointer& node) {
    return _pool._queue.try_pop(node);
}

#ifdef AUDIO_SINGLE_THREADED
static AudioMixerSlave slave;
#endif

void AudioMixerSlavePool::mix(ConstIter begin, ConstIter end, unsigned int frame) {
    _begin = begin;
    _end = end;
    _frame = frame;

#ifdef AUDIO_SINGLE_THREADED
    slave.configure(_begin, _end, frame);
    std::for_each(begin, end, [&](const SharedNodePointer& node) {
        slave.mix(node);
    });
#else
    // fill the queue
    std::for_each(_begin, _end, [&](const SharedNodePointer& node) {
        _queue.emplace(node);
    });

    {
        Lock lock(_mutex);

        // mix
        _numStarted = _numFinished = 0;
        _slaveCondition.notify_all();

        // wait
        _poolCondition.wait(lock, [&] {
            assert(_numFinished <= _numThreads);
            return _numFinished == _numThreads;
        });
    }
    assert(_numStarted == _numThreads);
    assert(_queue.empty());
#endif
}

void AudioMixerSlavePool::each(std::function<void(AudioMixerSlave& slave)> functor) {
#ifdef AUDIO_SINGLE_THREADED
    functor(slave);
#else
    for (auto& slave : _slaves) {
        functor(*slave.get());
    }
#endif
}

void AudioMixerSlavePool::setNumThreads(int numThreads) {
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
            qWarning("%s: clamped to %d (was %d)", __FUNCTION__, numThreads, clampedThreads);
            numThreads = clampedThreads;
        }
    }

    resize(numThreads);
}

void AudioMixerSlavePool::resize(int numThreads) {
    assert(_numThreads == _slaves.size());

#ifdef AUDIO_SINGLE_THREADED
    qDebug("%s: running single threaded", __FUNCTION__, numThreads);
#else
    qDebug("%s: set %d threads (was %d)", __FUNCTION__, numThreads, _numThreads);

    if (numThreads > _numThreads) {
        // start new slaves
        for (int i = 0; i < numThreads - _numThreads; ++i) {
            auto slave = new AudioMixerSlaveThread(*this);
            slave->start();
            _slaves.emplace_back(slave);
        }
    } else if (numThreads < _numThreads) {
        auto extraBegin = _slaves.begin() + numThreads;

        // stop extra slaves...
        auto slave = extraBegin;
        while (slave != _slaves.end()) {
            (*slave)->stop();
            ++slave;
        }

        // ...cycle slaves with empty queue...
        _numStarted = _numFinished = 0;
        _slaveCondition.notify_all();

        // ...wait for them to finish...
        slave = extraBegin;
        while (slave != _slaves.end()) {
            QThread* thread = reinterpret_cast<QThread*>(slave->get());
            static const int MAX_THREAD_WAIT_TIME = 10;
            thread->wait(MAX_THREAD_WAIT_TIME);
            ++slave;
        }

        // ...and delete them
        _slaves.erase(extraBegin, _slaves.end());
    }

    _numThreads = _numStarted = _numFinished = numThreads;
#endif

    assert(_numThreads == _slaves.size());
}
