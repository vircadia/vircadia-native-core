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

AudioInjector* AudioScriptingInterface::playSound(Sound* sound, const AudioInjectorOptions* injectorOptions) {
    
    if (sound->isStereo()) {
        const_cast<AudioInjectorOptions*>(injectorOptions)->setIsStereo(true);
    }
    AudioInjector* injector = new AudioInjector(sound, *injectorOptions);
    
    QThread* injectorThread = new QThread();
    
    injector->moveToThread(injectorThread);
    
    // start injecting when the injector thread starts
    connect(injectorThread, SIGNAL(started()), injector, SLOT(injectAudio()));
    
    // connect the right slots and signals so that the AudioInjector is killed once the injection is complete
    connect(injector, SIGNAL(finished()), injector, SLOT(deleteLater()));
    connect(injector, SIGNAL(finished()), injectorThread, SLOT(quit()));
    connect(injectorThread, SIGNAL(finished()), injectorThread, SLOT(deleteLater()));
    
    injectorThread->start();
    
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

void AudioScriptingInterface::startDrumSound(float volume, float frequency, float duration, float decay,
                                    const AudioInjectorOptions* injectorOptions) {

    Sound* sound = new Sound(volume, frequency, duration, decay);
    AudioInjector* injector = new AudioInjector(sound, *injectorOptions);
    sound->setParent(injector);
    
    QThread* injectorThread = new QThread();
    
    injector->moveToThread(injectorThread);
    
    // start injecting when the injector thread starts
    connect(injectorThread, SIGNAL(started()), injector, SLOT(injectAudio()));
    
    // connect the right slots and signals so that the AudioInjector is killed once the injection is complete
    connect(injector, SIGNAL(finished()), injector, SLOT(deleteLater()));
    connect(injector, SIGNAL(finished()), injectorThread, SLOT(quit()));
    connect(injectorThread, SIGNAL(finished()), injectorThread, SLOT(deleteLater()));
    
    injectorThread->start();
}
