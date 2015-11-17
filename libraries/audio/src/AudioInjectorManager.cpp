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

AudioInjectorManager::~AudioInjectorManager() {
    _shouldStop = true;
    
    std::unique_lock<std::mutex> lock(_injectorsMutex);
    
    // make sure any still living injectors are stopped and deleted
    while (!_injectors.empty()) {
        // grab the injector at the front
        auto& timePointerPair = _injectors.front();
        
        // ask it to stop and be deleted
        timePointerPair.second->stopAndDeleteLater();
        
        _injectors.pop();
    }
    
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
        std::unique_lock<std::mutex> lock(_injectorsMutex);
        
        if (_injectors.size() > 0) {
            // when does the next injector need to send a frame?
            // do we get to wait or should we just go for it now?
            
            auto timeInjectorPair = _injectors.front();
            
            auto nextTimestamp = timeInjectorPair.first;
            int64_t difference = int64_t(nextTimestamp - usecTimestampNow());
            
            if (difference > 0) {
                _injectorReady.wait_for(lock, std::chrono::microseconds(difference));
            }
            
            // loop through the injectors in the map and send whatever frames need to go out
            auto front = _injectors.front();
            while (_injectors.size() > 0 && front.first <= usecTimestampNow()) {
                // either way we're popping this injector off - get a copy first
                auto injector = front.second;
                _injectors.pop();
                
                if (!injector.isNull()) {
                    // this is an injector that's ready to go, have it send a frame now
                    auto nextCallDelta = injector->injectNextFrame();
                    
                    if (nextCallDelta > 0 && !injector->isFinished()) {
                        // re-enqueue the injector with the correct timing
                        _injectors.push({ usecTimestampNow() + nextCallDelta, injector });
                    }
                }
                
                front = _injectors.front();
            }
        } else {
            // we have no current injectors, wait until we get at least one before we do anything
            _injectorReady.wait(lock);
        }
        
        QCoreApplication::processEvents();
    }
}

static const int MAX_INJECTORS_PER_THREAD = 50; // calculated based on AudioInjector while loop time, with sufficient padding

bool AudioInjectorManager::threadInjector(AudioInjector* injector) {
    // guard the injectors vector with a mutex
    std::unique_lock<std::mutex> lock(_injectorsMutex);
    
    // check if we'll be able to thread this injector (do we have < max concurrent injectors)
    if (_injectors.size() < MAX_INJECTORS_PER_THREAD) {
        if (!_thread) {
            createThread();
        }
        
        // move the injector to the QThread
        injector->moveToThread(_thread);
        
        // handle a restart once the injector has finished
        connect(injector, &AudioInjector::restartedWhileFinished, this, &AudioInjectorManager::restartFinishedInjector);
        
        // store a QPointer to this injector
        addInjectorToQueue(injector);
        
        return true;
    } else {
        // unable to thread this injector, at the max
        qDebug() << "AudioInjectorManager::threadInjector could not thread AudioInjector - at max of"
            << MAX_INJECTORS_PER_THREAD << "current audio injectors.";
        return false;
    }
}

void AudioInjectorManager::restartFinishedInjector() {
    auto injector = qobject_cast<AudioInjector*>(sender());
    addInjectorToQueue(injector);
}

void AudioInjectorManager::addInjectorToQueue(AudioInjector* injector) {
    // add the injector to the queue with a send timestamp of now
    _injectors.emplace(usecTimestampNow(), InjectorQPointer { injector });
    
    // notify our wait condition so we can inject two frames for this injector immediately
    _injectorReady.notify_one();
}
