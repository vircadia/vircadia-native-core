//
//  AudioInjectorOptions.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "AudioInjectorOptions.h"

AudioInjectorOptions::AudioInjectorOptions(QObject* parent) :
    QObject(parent),
    _position(0.0f, 0.0f, 0.0f),
    _volume(1.0f),
    _orientation(glm::vec3(0.0f, 0.0f, 0.0f)),
    _loopbackAudioInterface(NULL)
{
    
}

AudioInjectorOptions::AudioInjectorOptions(const AudioInjectorOptions& other) {
    _position = other._position;
    _volume = other._volume;
    _orientation = other._orientation;
    _loopbackAudioInterface = other._loopbackAudioInterface;
}