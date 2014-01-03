//
//  AudioInjector.h
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjector__
#define __hifi__AudioInjector__

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AudioInjectorOptions.h"
#include "Sound.h"

class AbstractAudioInterface;
class AudioScriptingInterface;

class AudioInjector : public QObject {
    Q_OBJECT
public:
    AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions);
private:
    Sound* _sound;
    AudioInjectorOptions _options;
public slots:
    void injectAudio();
signals:
    void finished();
};

#endif /* defined(__hifi__AudioInjector__) */
