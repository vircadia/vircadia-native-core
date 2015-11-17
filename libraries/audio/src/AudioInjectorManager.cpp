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

#include "AudioInjector.h"

AudioInjectorManager::~AudioInjectorManager() {
    _shouldStop = true;
    
    // make sure any still living injectors are stopped and deleted
    for (auto injector : _injectors) {
        injector->stopAndDeleteLater();
    }
    
    // quit and wait on the injector thread, if we ever created it
    if (_thread) {
        _thread->quit();
        _thread->wait();
    }
}

void AudioInjectorManager::createThread() {
    _thread = new QThread;
    _thread->setObjectName("Audio Injector Thread");
    
    // when the thread is started, have it call our run to handle injection of audio
    connect(_thread, &QThread::started, this, &AudioInjectorManager::run);
    
    // start the thread
    _thread->start();
}

void AudioInjectorManager::run() {
    while (!_shouldStop) {
        // process events in case we have been told to stop or our injectors have been told to stop
        QCoreApplication::processEvents();
    }
}

static const int MAX_INJECTORS_PER_THREAD = 50; // calculated based on AudioInjector while loop time, with sufficient padding

bool AudioInjectorManager::threadInjector(AudioInjector* injector) {
    // guard the injectors vector with a mutex
    std::unique_lock<std::mutex> lock(_injectorsMutex);
    
    // check if we'll be able to thread this injector (do we have < MAX concurrent injectors)
    if (_injectors.size() < MAX_INJECTORS_PER_THREAD) {
        if (!_thread) {
            createThread();
        }
        
        auto it = nextInjectorIterator();
        qDebug() << "Inserting injector at" << it - _injectors.begin();
        
        // store a QPointer to this injector
        _injectors.insert(it, QPointer<AudioInjector>(injector));
        
        qDebug() << "Moving injector to thread" << _thread;
        
        // move the injector to the QThread
        injector->moveToThread(_thread);
        
        return true;
    } else {
        // unable to thread this injector, at the max
        qDebug() << "AudioInjectorManager::threadInjector could not thread AudioInjector - at max of"
            << MAX_INJECTORS_PER_THREAD << "current audio injectors.";
        return false;
    }
}

AudioInjectorVector::iterator AudioInjectorManager::nextInjectorIterator() {
    // find the next usable iterator for an injector
    auto it = _injectors.begin();
    
    while (it != _injectors.end()) {
        if (it->isNull()) {
            return it;
        } else {
            ++it;
        }
    }
    
    return it;
}
