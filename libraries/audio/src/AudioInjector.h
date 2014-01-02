//
//  AudioInjector.h
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjector__
#define __hifi__AudioInjector__

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Sound.h"

class AbstractAudioInterface;

struct AudioInjectorOptions {
    AudioInjectorOptions() : position(glm::vec3(0.0f, 0.0f, 0.0f)),
        volume(1.0f),
        orientation(glm::quat()),
        shouldLoopback(true),
        loopbackAudioInterface(NULL) {};
    
    glm::vec3 position;
    float volume;
    const glm::quat orientation;
    bool shouldLoopback;
    AbstractAudioInterface* loopbackAudioInterface;
};

class AudioInjector : public QObject {
    Q_OBJECT
public:
    static void threadSound(Sound* sound, AudioInjectorOptions injectorOptions = AudioInjectorOptions());
private:
    AudioInjector(Sound* sound, AudioInjectorOptions injectorOptions);
    
    QThread _thread;
    Sound* _sound;
    float _volume;
    uchar _shouldLoopback;
    glm::vec3 _position;
    glm::quat _orientation;
    AbstractAudioInterface* _loopbackAudioInterface;
private slots:
    void injectAudio();
signals:
    void finished();
};

#endif /* defined(__hifi__AudioInjector__) */
