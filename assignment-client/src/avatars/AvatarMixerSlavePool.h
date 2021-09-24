//
//  AvatarMixerSlavePool.h
//  assignment-client/src/avatar
//
//  Created by Brad Hefta-Gaub on 2/14/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarMixerSlavePool_h
#define hifi_AvatarMixerSlavePool_h

#include <condition_variable>
#include <mutex>
#include <vector>

#include <QThread>

#include <TBBHelpers.h>
#include <NodeList.h>
#include <shared/QtHelpers.h>

#include "AvatarMixerSlave.h"


// Private slave pool data that is shared and accessible with the slave threads.  This describes
// what information is needs to be thread-safe
struct AvatarMixerSlavePoolData {
    using Queue = tbb::concurrent_queue<SharedNodePointer>;
    using Mutex = std::mutex;
    using ConditionVariable = std::condition_variable;

    // synchronization state
    Mutex _poolMutex;                   // only used for _poolCondition at the moment
    ConditionVariable _poolCondition;   // woken when work has been completed (_numStarted = _numFinished = _numThreads)
    Mutex _slaveMutex;                  // only used for _slaveCondition at the moment
    ConditionVariable _slaveCondition;  // woken when work needs to be done (_numStarted < _numThreads)

    // The variables below this point are alternately owned by the pool or by the slaves collectively.
    // When idle they are owned by the pool.
    // Moving ownership to the slaves is done by setting _numStarted = _numFinished = 0 and waking _slaveCondition
    // Moving ownership to the pool is done when _numFinished == _numThreads and is done by waking _poolCondition

    void (AvatarMixerSlave::*_function)(const SharedNodePointer& node);   // r/o when owned by slaves, r/w when owned by pool
    std::function<void(AvatarMixerSlave&)> _configure;  // r/o when owned by slaves, r/w when owned by pool

    // Number of currently-running slave threads
    // r/o when owned by slaves, r/w when owned by pool
    int _numThreads{ 0 };

    // Number of slave threads "awake" and processing the current request (0 <= _numStarted <= _numThreads)
    // incremented when owned by slaves, r/w when owned by pool
    std::atomic<int> _numStarted{ 0 };

    // Number of slave threads finished with the current request (0 <= _numStarted <= _numThreads)
    // incremented when owned by slaves, r/w when owned by pool
    std::atomic<int> _numFinished{ 0 };

    // Number of slave threads shutting down when asked to (0 <= _numStarted <= _numThreads)
    // incremented when owned by slaves, r/w when owned by pool
    std::atomic<int> _numStopped{ 0 };

    // frame state
    Queue _queue;
};

class AvatarMixerSlaveThread : public QThread, public AvatarMixerSlave {
    Q_OBJECT
    using ConstIter = NodeList::const_iterator;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    AvatarMixerSlaveThread(AvatarMixerSlavePoolData& pool, SlaveSharedData* slaveSharedData) :
        AvatarMixerSlave(slaveSharedData), _pool(pool) {};

    void run() override final;
    inline void stop() { _stop = true; }

private:
    void wait();
    void notify(bool stopping);
    bool try_pop(SharedNodePointer& node);

    AvatarMixerSlavePoolData& _pool;
    void (AvatarMixerSlave::*_function)(const SharedNodePointer& node) { nullptr };
    bool _stop { false };
};

// Slave pool for avatar mixers
//   AvatarMixerSlavePool is not thread-safe! It should be instantiated and used from a single thread.
class AvatarMixerSlavePool : private AvatarMixerSlavePoolData {
    using Lock = std::unique_lock<Mutex>;

public:
    using ConstIter = NodeList::const_iterator;

    AvatarMixerSlavePool(SlaveSharedData* slaveSharedData, int numThreads = QThread::idealThreadCount()) :
        _slaveSharedData(slaveSharedData) { setNumThreads(numThreads); }
    ~AvatarMixerSlavePool() { resize(0); }

    // Jobs the slave pool can do...
    void processIncomingPackets(ConstIter begin, ConstIter end);
    void broadcastAvatarData(ConstIter begin, ConstIter end, 
                    p_high_resolution_clock::time_point lastFrameTimestamp, float maxKbpsPerNode, float throttlingRatio);

    // iterate over all slaves
    void each(std::function<void(AvatarMixerSlave& slave)> functor);

#ifdef DEBUG_EVENT_QUEUE
    void queueStats(QJsonObject& stats);
#endif

    void setNumThreads(int numThreads);
    int numThreads() const { return _numThreads; }

    void setPriorityReservedFraction(float fraction) { _priorityReservedFraction = fraction; }
    float getPriorityReservedFraction() const { return  _priorityReservedFraction; }

private:
    void run(ConstIter begin, ConstIter end);
    void resize(int numThreads);

    std::vector<std::unique_ptr<AvatarMixerSlaveThread>> _slaves;

    // Set from Domain Settings:
    float _priorityReservedFraction { 0.4f };

    // frame state
    ConstIter _begin;
    ConstIter _end;

    SlaveSharedData* _slaveSharedData;
};

#endif // hifi_AvatarMixerSlavePool_h
