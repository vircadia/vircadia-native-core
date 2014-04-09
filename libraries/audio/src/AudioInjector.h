//
//  AudioInjector.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AudioInjector__
#define __hifi__AudioInjector__

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AudioInjectorOptions.h"
#include "Sound.h"

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
