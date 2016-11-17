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

class AudioMixerSlaveThread;

class AudioMixerSlavePool {
    using Queue = tbb::concurrent_queue<SharedNodePointer>;

public:
    AudioMixerSlavePool(int numThreads = QThread::idealThreadCount()) { setNumThreads(numThreads); }
    ~AudioMixerSlavePool();

    // mix on slave threads
    void mix(const std::vector<SharedNodePointer>& nodes, unsigned int frame);

    void each(std::function<void(AudioMixerSlave& slave)> functor);

    void setNumThreads(int numThreads);
    int numThreads() { return _numThreads; }

private:
    void start(std::unique_lock<std::mutex>& lock, const std::vector<SharedNodePointer>& nodes, unsigned int frame);
    void wait(std::unique_lock<std::mutex>& lock);

    friend class AudioMixerSlaveThread;

    // wait for pool to start (called by slaves)
    void slaveWait();
    // notify that the slave has finished (called by slave)
    void slaveNotify();

    std::vector<std::unique_ptr<AudioMixerSlaveThread>> _slaves;
    Queue _queue;
    unsigned int _frame { 0 };

    std::mutex _mutex;
    std::condition_variable _slaveCondition;
    std::condition_variable _poolCondition;
    int _numThreads { 0 };
    int _numStarted { 0 };
    int _numFinished { 0 };
    bool _running { false };
};

class AudioMixerSlaveThread : public QThread, public AudioMixerSlave {
    Q_OBJECT
public:
    AudioMixerSlaveThread(AudioMixerSlavePool& pool) : _pool(pool) {}

    void run() override final;
    void stop() { _stop = true; }

private:
    AudioMixerSlavePool& _pool;
    std::atomic<bool> _stop;
};

#endif // hifi_AudioMixerSlavePool_h
