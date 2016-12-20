//
//  AudioInjectorManager.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2015-11-16.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioInjectorManager.h"

#include <QtCore/QCoreApplication>

#include <SharedUtil.h>

#include "AudioConstants.h"
#include "AudioInjector.h"
#include "AudioLogging.h"

AudioInjectorManager::~AudioInjectorManager() {
    _shouldStop = true;
    
    Lock lock(_injectorsMutex);
    
    // make sure any still living injectors are stopped and deleted
    while (!_injectors.empty()) {
        // grab the injector at the front
        auto& timePointerPair = _injectors.top();
        
        // ask it to stop and be deleted
        timePointerPair.second->stopAndDeleteLater();
        
        _injectors.pop();
    }
    
    // get rid of the lock now that we've stopped all living injectors
    lock.unlock();
    
    // in case the thread is waiting for injectors wake it up now
    _injectorReady.notify_one();
    
    // quit and wait on the manager thread, if we ever created it
    if (_thread) {
        _thread->quit();
        _thread->wait();
    }
}

void AudioInjectorManager::createThread() {
    _thread = new QThread;
    _thread->setObjectName("Audio Injector Thread");
    
    // when the thread is started, have it call our run to handle injection of audio
    connect(_thread, &QThread::started, this, &AudioInjectorManager::run, Qt::DirectConnection);
    
    // start the thread
    _thread->start();
}

void AudioInjectorManager::run() {
    while (!_shouldStop) {
        // wait until the next injector is ready, or until we get a new injector given to us
        Lock lock(_injectorsMutex);
        
        if (_injectors.size() > 0) {
            // when does the next injector need to send a frame?
            // do we get to wait or should we just go for it now?
            
            auto timeInjectorPair = _injectors.top();
            
            auto nextTimestamp = timeInjectorPair.first;
            int64_t difference = int64_t(nextTimestamp - usecTimestampNow());
            
            if (difference > 0) {
                _injectorReady.wait_for(lock, std::chrono::microseconds(difference));
            }
            
            if (_injectors.size() > 0) {
                // loop through the injectors in the map and send whatever frames need to go out
                auto front = _injectors.top();

                // create an InjectorQueue to hold injectors to be queued
                // this allows us to call processEvents even if a single injector wants to be re-queued immediately
                std::vector<TimeInjectorPointerPair> heldInjectors;
                heldInjectors.reserve(_injectors.size());

                while (_injectors.size() > 0 && front.first <= usecTimestampNow()) {
                    // either way we're popping this injector off - get a copy first
                    auto injector = front.second;
                    _injectors.pop();
                    
                    if (!injector.isNull()) {
                        // this is an injector that's ready to go, have it send a frame now
                        auto nextCallDelta = injector->injectNextFrame();

                        if (nextCallDelta >= 0 && !injector->isFinished()) {
                            // enqueue the injector with the correct timing in our holding queue
                            heldInjectors.emplace(heldInjectors.end(), usecTimestampNow() + nextCallDelta, injector);
                        }
                    }
                    
                    if (_injectors.size() > 0) {
                        front = _injectors.top();
                    } else {
                        // no more injectors to look at, break
                        break;
                    }
                }

                // if there are injectors in the holding queue, push them to our persistent queue now
                while (!heldInjectors.empty()) {
                    _injectors.push(heldInjectors.back());
                    heldInjectors.pop_back();
                }
            }

        } else {
            // we have no current injectors, wait until we get at least one before we do anything
            _injectorReady.wait(lock);
        }
        
        // unlock the lock in case something in process events needs to modify the queue
        lock.unlock();
        
        QCoreApplication::processEvents();
    }
}

static const int MAX_INJECTORS_PER_THREAD = 40; // calculated based on AudioInjector time to send frame, with sufficient padding

bool AudioInjectorManager::wouldExceedLimits() { // Should be called inside of a lock.
    if (_injectors.size() >= MAX_INJECTORS_PER_THREAD) {
        qCDebug(audio)  << "AudioInjectorManager::threadInjector could not thread AudioInjector - at max of"
            << MAX_INJECTORS_PER_THREAD << "current audio injectors.";
        return true;
    }
    return false;
}

bool AudioInjectorManager::threadInjector(AudioInjector* injector) {
    if (_shouldStop) {
        qCDebug(audio)  << "AudioInjectorManager::threadInjector asked to thread injector but is shutting down.";
        return false;
    }
    
    // guard the injectors vector with a mutex
    Lock lock(_injectorsMutex);
    
    if (wouldExceedLimits()) {
        return false;
    } else {
        if (!_thread) {
            createThread();
        }
        
        // move the injector to the QThread
        injector->moveToThread(_thread);
        
        // add the injector to the queue with a send timestamp of now
        _injectors.emplace(usecTimestampNow(), InjectorQPointer { injector });
        
        // notify our wait condition so we can inject two frames for this injector immediately
        _injectorReady.notify_one();
        
        return true;
    }
}

bool AudioInjectorManager::restartFinishedInjector(AudioInjector* injector) {
    if (_shouldStop) {
        qCDebug(audio)  << "AudioInjectorManager::threadInjector asked to thread injector but is shutting down.";
        return false;
    }

    // guard the injectors vector with a mutex
    Lock lock(_injectorsMutex);

    if (wouldExceedLimits()) {
        return false;
    } else {
        // add the injector to the queue with a send timestamp of now
        _injectors.emplace(usecTimestampNow(), InjectorQPointer { injector });
        
        // notify our wait condition so we can inject two frames for this injector immediately
        _injectorReady.notify_one();
    }
    return true;
}
