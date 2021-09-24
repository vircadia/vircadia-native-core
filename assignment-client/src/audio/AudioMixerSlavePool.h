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
#include <shared/QtHelpers.h>
#include <TBBHelpers.h>
#include <SharedMutex.h>

#include "AudioMixerSlave.h"

// Private slave pool data that is shared and accessible with the slave threads.  This describes
// what information is needs to be thread-safe
struct AudioMixerSlavePoolData {
    using Queue = tbb::concurrent_queue<SharedNodePointer>;
    using Mutex = std::mutex;
    using ConditionVariable = std::condition_variable;
    using RWMutex = shared_mutex;

    // synchronization state
    shared_mutex _slavesActive;  // shared_lock by slaves while running, lock by pool when configuring
    Mutex _poolMutex;            // only used for _poolCondition at the moment
    ConditionVariable _poolCondition;
    Mutex _slaveMutex;  // only used for _slaveCondition at the moment
    ConditionVariable _slaveCondition;

    void (AudioMixerSlave::*_function)(const SharedNodePointer& node);  // guarded by _slavesActive: r/o when shared, r/w when locked
    std::function<void(AudioMixerSlave&)> _configure;  // guarded by _slavesActive: r/o when shared, r/w when locked

    // Number of currently-running slave threads
    // guarded by _slavesActive: r/o when shared, r/w when locked
    int _numThreads{ 0 };

    // Number of slave threads "awake" and processing the current request (0 <= _numStarted <= _numThreads)
    // guarded by _slavesActive: incremented when shared, r/w when locked
    std::atomic<int> _numStarted{ 0 };

    // Number of slave threads finished with the current request (0 <= _numStarted <= _numThreads)
    // guarded by _slavesActive: incremented when shared, r/w when locked
    std::atomic<int> _numFinished{ 0 };

    // Number of slave threads shutting down when asked to (0 <= _numStarted <= _numThreads)
    // guarded by _slavesActive: incremented when shared, r/w when locked
    std::atomic<int> _numStopped{ 0 };

    // frame state
    Queue _queue;
};

class AudioMixerSlaveThread : public QThread, public AudioMixerSlave {
    Q_OBJECT
    using ConstIter = NodeList::const_iterator;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using RWMutex = shared_mutex;

public:
    AudioMixerSlaveThread(AudioMixerSlavePoolData& pool, AudioMixerSlave::SharedData& sharedData)
        : AudioMixerSlave(sharedData), _pool(pool) {}

    void run() override final;
    inline void stop() { _stop = true; }

private:
    void wait();
    void notify(bool stopping);
    bool try_pop(SharedNodePointer& node);

    AudioMixerSlavePoolData& _pool;
    void (AudioMixerSlave::*_function)(const SharedNodePointer& node) { nullptr };
    bool _stop { false };
};

// Slave pool for audio mixers
//   AudioMixerSlavePool is not thread-safe! It should be instantiated and used from a single thread.
class AudioMixerSlavePool : private AudioMixerSlavePoolData {
    using Lock = std::unique_lock<Mutex>;

public:
    using ConstIter = NodeList::const_iterator;

    AudioMixerSlavePool(AudioMixerSlave::SharedData& sharedData, int numThreads = QThread::idealThreadCount())
        : _workerSharedData(sharedData) { setNumThreads(numThreads); }
    ~AudioMixerSlavePool() { resize(0); }

    // process packets on slave threads
    void processPackets(ConstIter begin, ConstIter end);

    // mix on slave threads
    void mix(ConstIter begin, ConstIter end, unsigned int frame, int numToRetain);

    // iterate over all slaves
    void each(std::function<void(AudioMixerSlave& slave)> functor);

#ifdef DEBUG_EVENT_QUEUE
    void queueStats(QJsonObject& stats);
#endif

    void setNumThreads(int numThreads);
    int numThreads() { return _numThreads; }

private:
    void run(ConstIter begin, ConstIter end);
    void resize(int numThreads);

    std::vector<std::unique_ptr<AudioMixerSlaveThread>> _slaves;

    // frame state
    ConstIter _begin;
    ConstIter _end;

    AudioMixerSlave::SharedData& _workerSharedData;
};

#endif // hifi_AudioMixerSlavePool_h
