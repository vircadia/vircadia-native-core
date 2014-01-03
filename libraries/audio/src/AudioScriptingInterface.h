//
//  AudioScriptingInterface.h
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioScriptingInterface__
#define __hifi__AudioScriptingInterface__

#include "AudioInjector.h"
#include "Sound.h"

const AudioInjectorOptions DEFAULT_INJECTOR_OPTIONS;

class AudioScriptingInterface : public QObject {
    Q_OBJECT
public slots:
    static void playSound(Sound* sound, AudioInjectorOptions injectorOptions = DEFAULT_INJECTOR_OPTIONS);

};
#endif /* defined(__hifi__AudioScriptingInterface__) */
