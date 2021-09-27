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
#include <shared/QtHelpers.h> // for DEBUG_EVENT_QUEUE
#include <TBBHelpers.h>

#include "AudioMixerSlave.h"

// Private worker pool data that is shared and accessible with the worker threads.  This describes
// what information is needs to be thread-safe
struct AudioMixerWorkerPoolData {
    using Queue = tbb::concurrent_queue<SharedNodePointer>;
    using Mutex = std::mutex;
    using ConditionVariable = std::condition_variable;

    // synchronization state
    Mutex poolMutex;                   // only used for _poolCondition at the moment
    ConditionVariable poolCondition;   // woken when work has been completed (_numStarted = _numFinished = _numThreads)
    Mutex workerMutex;                 // only used for _workerCondition at the moment
    ConditionVariable workerCondition; // woken when work needs to be done (_numStarted < _numThreads)

    // The variables below this point are alternately owned by the pool or by the workers collectively.
    // When idle they are owned by the pool.
    // Moving ownership to the workers is done by setting _numStarted = _numFinished = 0 and waking _workerCondition
    // Moving ownership to the pool is done when _numFinished == _numThreads and is done by waking _poolCondition

    void (AudioMixerSlave::*function)(const SharedNodePointer& node){ nullptr };  // r/o when owned by workers, r/w when owned by pool
    std::function<void(AudioMixerSlave&)> configure{ nullptr };  // r/o when owned by workers, r/w when owned by pool

    // Number of currently-running worker threads
    // r/o when owned by workersves, r/w when owned by pool
    int numThreads{ 0 };

    // Number of worker threads "awake" and processing the current request (0 <= _numStarted <= _numThreads)
    // incremented when owned by workers, r/w when owned by pool
    std::atomic<int> numStarted{ 0 };

    // Number of worker threads finished with the current request (0 <= _numFinished <= _numStarted)
    // incremented when owned by workers, r/w when owned by pool
    std::atomic<int> numFinished{ 0 };

    // Number of worker threads shutting down when asked to (0 <= _numStopped <= _numFinished)
    // incremented when owned by workers, r/w when owned by pool
    std::atomic<int> numStopped{ 0 };

    // frame state
    Queue queue;
};

class AudioMixerWorkerThread : public QThread, public AudioMixerSlave {
    Q_OBJECT
    using ConstIter = NodeList::const_iterator;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    AudioMixerWorkerThread(AudioMixerWorkerPoolData& data, AudioMixerSlave::SharedData& sharedData) :
        AudioMixerSlave(sharedData), _data(data) {}

    void run() override final;
    inline void stop() { _stop = true; }

private:
    void wait();
    void notify(bool stopping);
    bool try_pop(SharedNodePointer& node);

    AudioMixerWorkerPoolData& _data;
    void (AudioMixerSlave::*_function)(const SharedNodePointer& node) { nullptr };
    volatile bool _stop { false }; // using volatile here mostly for compiler hinting, recognize it has minimal meaning
};

// Worker pool for audio mixers
//   AudioMixerSlavePool is not thread-safe! It should be instantiated and used from a single thread.
class AudioMixerSlavePool {
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    using ConstIter = NodeList::const_iterator;

    AudioMixerSlavePool(AudioMixerSlave::SharedData& sharedData, int numThreads = QThread::idealThreadCount())
        : _workerSharedData(sharedData) { setNumThreads(numThreads); }
    ~AudioMixerSlavePool() { resize(0); }

    // process packets on worker threads
    void processPackets(ConstIter begin, ConstIter end);

    // mix on worker threads
    void mix(ConstIter begin, ConstIter end, unsigned int frame, int numToRetain);

    // iterate over all workers
    void each(std::function<void(AudioMixerSlave& slave)> functor);

#ifdef DEBUG_EVENT_QUEUE
    void queueStats(QJsonObject& stats);
#endif

    void setNumThreads(int numThreads);
    int numThreads() { return _data.numThreads; }

private:
    void run(ConstIter begin, ConstIter end);
    void resize(int numThreads);

    std::vector<std::unique_ptr<AudioMixerWorkerThread>> _workers;

    // frame state
    ConstIter _begin;
    ConstIter _end;

    AudioMixerSlave::SharedData& _workerSharedData;
    AudioMixerWorkerPoolData _data;
};

#endif // hifi_AudioMixerSlavePool_h
