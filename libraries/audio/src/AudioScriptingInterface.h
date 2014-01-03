//
//  AudioScriptingInterface.h
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
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

};
#endif /* defined(__hifi__AudioScriptingInterface__) */
