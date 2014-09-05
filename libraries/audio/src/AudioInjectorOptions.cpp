//
//  AudioInjectorOptions.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioInjectorOptions.h"

AudioInjectorOptions::AudioInjectorOptions(QObject* parent) :
    QObject(parent),
    _position(0.0f, 0.0f, 0.0f),
    _volume(1.0f),
    _loop(false),
    _orientation(glm::vec3(0.0f, 0.0f, 0.0f)),
    _isStereo(false),
    _loopbackAudioInterface(NULL)
{
}

AudioInjectorOptions::AudioInjectorOptions(const AudioInjectorOptions& other) {
    _position = other._position;
    _volume = other._volume;
    _loop = other._loop;
    _orientation = other._orientation;
    _isStereo = other._isStereo;
    _loopbackAudioInterface = other._loopbackAudioInterface;
}

void AudioInjectorOptions::operator=(const AudioInjectorOptions& other) {
    _position = other._position;
    _volume = other._volume;
    _loop = other._loop;
    _orientation = other._orientation;
    _isStereo = other._isStereo;
    _loopbackAudioInterface = other._loopbackAudioInterface;
}