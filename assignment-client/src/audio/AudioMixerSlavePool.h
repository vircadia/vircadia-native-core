//
//  AudioMixerSlavePool.h
//  assignment-client/src/audio
//
//  Created by Zach Pomerantz on 11/16/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixerSlavePool_h
#define hifi_AudioMixerSlavePool_h

#include <condition_variable>
#include <mutex>
#include <vector>

#include <tbb/concurrent_queue.h>

#include <QThread>

#include "AudioMixerSlave.h"

class AudioMixerSlavePool;

class AudioMixerSlaveThread : public QThread, public AudioMixerSlave {
    Q_OBJECT
    using ConstIter = NodeList::const_iterator;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    AudioMixerSlaveThread(AudioMixerSlavePool& pool) : _pool(pool) {}

    void run() override final;

private:
    friend class AudioMixerSlavePool;

    void wait();
    void notify(bool stopping);
    bool try_pop(SharedNodePointer& node);

    AudioMixerSlavePool& _pool;
    bool _stop;
};

// Slave pool for audio mixers
//   AudioMixerSlavePool is not thread-safe! It should be instantiated and used from a single thread.
class AudioMixerSlavePool {
    using Queue = tbb::concurrent_queue<SharedNodePointer>;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using ConditionVariable = std::condition_variable;

public:
    using ConstIter = NodeList::const_iterator;

    AudioMixerSlavePool(int numThreads = QThread::idealThreadCount()) { setNumThreads(numThreads); }
    ~AudioMixerSlavePool() { resize(0); }

    // mix on slave threads
    void mix(ConstIter begin, ConstIter end, unsigned int frame);

    // iterate over all slaves
    void each(std::function<void(AudioMixerSlave& slave)> functor);

    void setNumThreads(int numThreads);
    int numThreads() { return _numThreads; }

private:
    void resize(int numThreads);

    std::vector<std::unique_ptr<AudioMixerSlaveThread>> _slaves;

    friend void AudioMixerSlaveThread::wait();
    friend void AudioMixerSlaveThread::notify(bool stopping);
    friend bool AudioMixerSlaveThread::try_pop(SharedNodePointer& node);

    // synchronization state
    Mutex _mutex;
    ConditionVariable _slaveCondition;
    ConditionVariable _poolCondition;
    int _numThreads { 0 };
    int _numStarted { 0 }; // guarded by _mutex
    int _numFinished { 0 }; // guarded by _mutex
    int _numStopped { 0 }; // guarded by _mutex

    // frame state
    Queue _queue;
    unsigned int _frame { 0 };
    ConstIter _begin;
    ConstIter _end;
};

#endif // hifi_AudioMixerSlavePool_h
