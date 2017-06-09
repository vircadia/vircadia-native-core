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

#include <QThread>

#include <TBBHelpers.h>

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
    void (AudioMixerSlave::*_function)(const SharedNodePointer& node) { nullptr };
    bool _stop { false };
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

    // process packets on slave threads
    void processPackets(ConstIter begin, ConstIter end);

    // mix on slave threads
    void mix(ConstIter begin, ConstIter end, unsigned int frame, float throttlingRatio);

    // iterate over all slaves
    void each(std::function<void(AudioMixerSlave& slave)> functor);

    void setNumThreads(int numThreads);
    int numThreads() { return _numThreads; }

private:
    void run(ConstIter begin, ConstIter end);
    void resize(int numThreads);

    std::vector<std::unique_ptr<AudioMixerSlaveThread>> _slaves;

    friend void AudioMixerSlaveThread::wait();
    friend void AudioMixerSlaveThread::notify(bool stopping);
    friend bool AudioMixerSlaveThread::try_pop(SharedNodePointer& node);

    // synchronization state
    Mutex _mutex;
    ConditionVariable _slaveCondition;
    ConditionVariable _poolCondition;
    void (AudioMixerSlave::*_function)(const SharedNodePointer& node);
    std::function<void(AudioMixerSlave&)> _configure;
    int _numThreads { 0 };
    int _numStarted { 0 }; // guarded by _mutex
    int _numFinished { 0 }; // guarded by _mutex
    int _numStopped { 0 }; // guarded by _mutex

    // frame state
    Queue _queue;
    unsigned int _frame { 0 };
    float _throttlingRatio { 0.0f };
    ConstIter _begin;
    ConstIter _end;
};

#endif // hifi_AudioMixerSlavePool_h
