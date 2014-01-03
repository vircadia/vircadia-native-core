//
//  AudioScriptingInterface.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "AudioScriptingInterface.h"

void AudioScriptingInterface::playSound(Sound* sound, const AudioInjectorOptions* injectorOptions) {
    
    AudioInjector* injector = new AudioInjector(sound, *injectorOptions);
    
    QThread* injectorThread = new QThread();
    
    // start injecting when the injector thread starts
    connect(injectorThread, SIGNAL(started()), injector, SLOT(injectAudio()));
    
    // connect the right slots and signals so that the AudioInjector is killed once the injection is complete
    connect(injector, SIGNAL(finished()), injector, SLOT(deleteLater()));
    connect(injector, SIGNAL(finished()), injectorThread, SLOT(quit()));
    connect(injectorThread, SIGNAL(finished()), injectorThread, SLOT(deleteLater()));
    
    injectorThread->start();
}