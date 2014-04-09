//
//  AudioScriptingInterface.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AudioScriptingInterface__
#define __hifi__AudioScriptingInterface__

#include "AudioInjector.h"
#include "Sound.h"

const AudioInjectorOptions DEFAULT_INJECTOR_OPTIONS;

class AudioScriptingInterface : public QObject {
    Q_OBJECT
public slots:
    static void playSound(Sound* sound, const AudioInjectorOptions* injectorOptions = NULL);
    static void startDrumSound(float volume, float frequency, float duration, float decay, 
                    const AudioInjectorOptions* injectorOptions = NULL);

};
#endif /* defined(__hifi__AudioScriptingInterface__) */
