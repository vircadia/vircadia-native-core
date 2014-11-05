//
//  AudioScriptingInterface.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioScriptingInterface.h"

AudioScriptingInterface& AudioScriptingInterface::getInstance() {
    static AudioScriptingInterface staticInstance;
    return staticInstance;
}

void AudioScriptingInterface::stopAllInjectors() {
    
}

AudioInjector* AudioScriptingInterface::playSound(Sound* sound, const AudioInjectorOptions* injectorOptions) {
    
    if (sound->isStereo()) {
        const_cast<AudioInjectorOptions*>(injectorOptions)->setIsStereo(true);
    }
    AudioInjector* injector = new AudioInjector(sound, *injectorOptions);
    
    QThread* injectorThread = new QThread();
    
    injector->moveToThread(injectorThread);
    
    // start injecting when the injector thread starts
    connect(injectorThread, &QThread::started, injector, &AudioInjector::injectAudio);
    
    // connect the right slots and signals so that the AudioInjector is killed once the injection is complete
    connect(injector, &AudioInjector::finished, injector, &AudioInjector::deleteLater);
    connect(injector, &AudioInjector::finished, injectorThread, &QThread::quit);
    connect(injector, &AudioInjector::finished, this, &AudioScriptingInterface::injectorStopped);
    connect(injectorThread, &QThread::finished, injectorThread, &QThread::deleteLater);
    
    injectorThread->start();
    
    _activeInjectors.insert(injector);
    
    return injector;
}

void AudioScriptingInterface::stopInjector(AudioInjector* injector) {
    if (injector) {
        injector->stop();
    }
}

bool AudioScriptingInterface::isInjectorPlaying(AudioInjector* injector) {
    return (injector != NULL);
}

void AudioScriptingInterface::injectorStopped() {
    qDebug() << "Removing" << sender() << "from active injectors";
    qDebug() << _activeInjectors.size();
    _activeInjectors.remove(static_cast<AudioInjector*>(sender()));
    qDebug() << _activeInjectors.size();
}