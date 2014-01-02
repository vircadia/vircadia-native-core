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

class AudioInjector : public QObject {
public:
    AudioInjector(Sound* sound);
    
    void setPosition(const glm::vec3& position) { _position = position; }
    void setOrientation(const glm::quat& orientation) { _orientation = orientation; }
    void setVolume(float volume) { _volume = std::max(fabsf(volume), 1.0f); }
    void setShouldLoopback(bool shouldLoopback) { _shouldLoopback = shouldLoopback; }
public slots:
    void injectViaThread(AbstractAudioInterface* localAudioInterface = NULL);
private:
    QThread _thread;
    Sound* _sound;
    float _volume;
    uchar _shouldLoopback;
    glm::vec3 _position;
    glm::quat _orientation;
private slots:
    void injectAudio(AbstractAudioInterface* localAudioInterface);
};

#endif /* defined(__hifi__AudioInjector__) */
