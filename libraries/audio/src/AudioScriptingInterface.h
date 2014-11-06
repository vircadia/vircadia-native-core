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

#ifndef hifi_AudioScriptingInterface_h
#define hifi_AudioScriptingInterface_h

#include "AudioInjector.h"
#include "Sound.h"

const AudioInjectorOptions DEFAULT_INJECTOR_OPTIONS;

class AudioScriptingInterface : public QObject {
    Q_OBJECT
public slots:
    static AudioInjector* playSound(Sound* sound, const AudioInjectorOptions* injectorOptions = NULL);
    static void stopInjector(AudioInjector* injector);
    static bool isInjectorPlaying(AudioInjector* injector);
    static float getLoudness(AudioInjector* injector);
    static void startDrumSound(float volume, float frequency, float duration, float decay, 
                    const AudioInjectorOptions* injectorOptions = NULL);

};
#endif // hifi_AudioScriptingInterface_h
