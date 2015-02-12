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

#include "ScriptAudioInjector.h"

#include "AudioScriptingInterface.h"

void registerAudioMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, injectorOptionsToScriptValue, injectorOptionsFromScriptValue);
    qScriptRegisterMetaType(engine, soundSharedPointerToScriptValue, soundSharedPointerFromScriptValue);
    qScriptRegisterMetaType(engine, soundPointerToScriptValue, soundPointerFromScriptValue);
}

AudioScriptingInterface& AudioScriptingInterface::getInstance() {
    static AudioScriptingInterface staticInstance;
    return staticInstance;
}

AudioScriptingInterface::AudioScriptingInterface() :
    _localAudioInterface(NULL)
{
    
}

ScriptAudioInjector* AudioScriptingInterface::playSound(Sound* sound, const AudioInjectorOptions& injectorOptions) {
    AudioInjector* injector = NULL;
    QMetaObject::invokeMethod(this, "invokedPlaySound", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(AudioInjector*, injector),
                              Q_ARG(Sound*, sound), Q_ARG(const AudioInjectorOptions&, injectorOptions));
    if (injector) {
        return new ScriptAudioInjector(injector);
    } else {
        return NULL;
    }
}

AudioInjector* AudioScriptingInterface::invokedPlaySound(Sound* sound, const AudioInjectorOptions& injectorOptions) {
    if (sound) {
        // stereo option isn't set from script, this comes from sound metadata or filename
        AudioInjectorOptions optionsCopy = injectorOptions;
        optionsCopy.stereo = sound->isStereo();
        
        QThread* injectorThread = new QThread();
        injectorThread->setObjectName("Audio Injector Thread");
        
        AudioInjector* injector = new AudioInjector(sound, optionsCopy);
        injector->setLocalAudioInterface(_localAudioInterface);
        
        injector->moveToThread(injectorThread);
        
        // start injecting when the injector thread starts
        connect(injectorThread, &QThread::started, injector, &AudioInjector::injectAudio);
        
        // connect the right slots and signals for AudioInjector and thread cleanup
        connect(injector, &AudioInjector::destroyed, injectorThread, &QThread::quit);
        connect(injectorThread, &QThread::finished, injectorThread, &QThread::deleteLater);
        
        injectorThread->start();
        
        return injector;
        
    } else {
        qDebug() << "AudioScriptingInterface::playSound called with null Sound object.";
        return NULL;
    }
}
