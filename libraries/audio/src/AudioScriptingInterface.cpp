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

void registerAudioMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, injectorOptionsToScriptValue, injectorOptionsFromScriptValue);
    qScriptRegisterMetaType(engine, soundToScriptValue, soundFromScriptValue);
}

AudioScriptingInterface& AudioScriptingInterface::getInstance() {
    static AudioScriptingInterface staticInstance;
    return staticInstance;
}

AudioScriptingInterface::AudioScriptingInterface() :
    _localLoopbackInterface(NULL)
{
    
}

void AudioScriptingInterface::stopAllInjectors() {
    QList<QPointer<AudioInjector> >::iterator injector = _activeInjectors.begin();
    while (injector != _activeInjectors.end()) {
        if (!injector->isNull()) {
            injector->data()->stop();
            
            while (injector->data() && !injector->data()->isFinished()) {
                // wait for this injector to go down
            }
        }
        
        injector = _activeInjectors.erase(injector);
    }
}

AudioInjector* AudioScriptingInterface::playSound(Sound* sound, AudioInjectorOptions& injectorOptions) {
    
    AudioInjector* injector = new AudioInjector(sound, injectorOptions);
    
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
    
    _activeInjectors.append(QPointer<AudioInjector>(injector));
    
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

float AudioScriptingInterface::getLoudness(AudioInjector* injector) {
    if (injector) {
        return injector->getLoudness();
    } else {
        return 0.0f;
    }
}

void AudioScriptingInterface::injectorStopped() {
    _activeInjectors.removeAll(QPointer<AudioInjector>(reinterpret_cast<AudioInjector*>(sender())));
}
