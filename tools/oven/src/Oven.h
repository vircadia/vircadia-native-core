//
//  Oven.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Oven_h
#define hifi_Oven_h

#include <atomic>
#include <memory>
#include <vector>

class QThread;

class Oven {

public:
    Oven();
    ~Oven();

    static Oven& instance() { return *_staticInstance; }

    QThread* getNextWorkerThread();

private:
    void setupWorkerThreads(int numWorkerThreads);
    void setupFBXBakerThread();

    std::vector<std::unique_ptr<QThread>> _workerThreads;

    std::atomic<uint32_t> _nextWorkerThreadIndex;
    int _numWorkerThreads;

    static Oven* _staticInstance;
};


#endif // hifi_Oven_h
