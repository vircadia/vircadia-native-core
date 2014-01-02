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

class AudioScriptingInterface : public QObject {
public slots:
    static void playSound(Sound* sound, AudioInjectorOptions injectorOptions = AudioInjectorOptions());

};
#endif /* defined(__hifi__AudioScriptingInterface__) */
