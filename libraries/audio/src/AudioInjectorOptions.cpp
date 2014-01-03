//
//  AudioInjectorOptions.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "AudioInjectorOptions.h"

AudioInjectorOptions::AudioInjectorOptions(QObject* parent) :
    QObject(parent),
    position(0.0f, 0.0f, 0.0f),
    volume(1.0f),
    orientation(glm::vec3(0.0f, 0.0f, 0.0f)),
    shouldLoopback(true),
    loopbackAudioInterface(NULL)
{
    
}

AudioInjectorOptions::AudioInjectorOptions(const AudioInjectorOptions& other) {
    position = other.position;
    volume = other.volume;
    orientation = other.orientation;
    shouldLoopback = other.shouldLoopback;
    loopbackAudioInterface = other.loopbackAudioInterface;
}