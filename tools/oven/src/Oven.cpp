//
//  Oven.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Oven.h"

#include <QtCore/QDebug>
#include <QtCore/QThread>

#include <image/TextureProcessing.h>

#include <DependencyManager.h>
#include <StatTracker.h>
#include <ResourceManager.h>
#include <ResourceRequestObserver.h>
#include <ResourceCache.h>
#include <procedural/ProceduralMaterialCache.h>
#include <material-networking/TextureCache.h>
#include <hfm/ModelFormatRegistry.h>
#include <FBXSerializer.h>
#include <OBJSerializer.h>

#include "MaterialBaker.h"

Oven* Oven::_staticInstance { nullptr };

Oven::Oven() {
    _staticInstance = this;

    // setup our worker threads
    setupWorkerThreads(QThread::idealThreadCount());

    // Initialize dependencies for OBJ Baker
    DependencyManager::set<StatTracker>();
    DependencyManager::set<ResourceManager>(false);
    DependencyManager::set<ResourceRequestObserver>();
    DependencyManager::set<ResourceCacheSharedItems>();
    DependencyManager::set<TextureCache>();
    DependencyManager::set<MaterialCache>();

    MaterialBaker::setNextOvenWorkerThreadOperator([] {
        return Oven::instance().getNextWorkerThread();
    });

    {
        auto modelFormatRegistry = DependencyManager::set<ModelFormatRegistry>();
        modelFormatRegistry->addFormat(FBXSerializer());
        modelFormatRegistry->addFormat(OBJSerializer());
    }
}

Oven::~Oven() {
    DependencyManager::get<ResourceManager>()->cleanup();

    // quit all worker threads and wait on them
    for (auto& thread : _workerThreads) {
        thread->quit();
    }

    for (auto& thread: _workerThreads) {
        thread->wait();
    }

    _staticInstance = nullptr;
}

void Oven::setupWorkerThreads(int numWorkerThreads) {
    _workerThreads.reserve(numWorkerThreads);

    for (auto i = 0; i < numWorkerThreads; ++i) {
        // setup a worker thread yet and add it to our concurrent vector
        auto newThread = std::unique_ptr<QThread> { new QThread };
        newThread->setObjectName("Oven Worker Thread " + QString::number(i + 1));

        _workerThreads.push_back(std::move(newThread));
    }
}

QThread* Oven::getNextWorkerThread() {
    // FIXME: we assign these threads when we make the bakers, but if certain bakers finish quickly, we could end up
    // in a situation where threads have finished and others have tons of work queued.  Instead of assigning them at initialization,
    // we should build a queue of bakers, and when threads finish, they can take the next available baker.

    // Here we replicate some of the functionality of QThreadPool by giving callers an available worker thread to use.
    // We can't use QThreadPool because we want to put QObjects with signals/slots on these threads.
    // So instead we setup our own list of threads, up to one less than the ideal thread count
    // (for the FBX Baker Thread to have room), and cycle through them to hand a usable running thread back to our callers.

    auto nextIndex = ++_nextWorkerThreadIndex;
    auto& nextThread = _workerThreads[nextIndex % _workerThreads.size()];

    // start the thread if it isn't running yet
    if (!nextThread->isRunning()) {
        nextThread->start();
    }

    return nextThread.get();
}

