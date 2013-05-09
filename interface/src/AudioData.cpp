//
//  AudioData.cpp
//  interface
//
//  Created by Stephen Birarda on 1/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
#ifndef _WIN32

#include "AudioData.h"

AudioData::AudioData() {
    mixerAddress = 0;
    mixerPort = 0;
    
    averagedLatency = 0.0;
    lastCallback.tv_usec = 0;
    wasStarved = 0;
    measuredJitter = 0;
    jitterBuffer = 0;
    
    mixerLoopbackFlag = false;
    audioSocket = NULL;
}


AudioData::~AudioData() {    
    delete audioSocket;
}

//  Take a pointer to the acquired microphone input samples and add procedural sounds
void AudioData::addProceduralSounds(int16_t* inputBuffer, int numSamples) {
    const float MAX_AUDIBLE_VELOCITY = 3.0;
    const float MIN_AUDIBLE_VELOCITY = 0.1;
    const float VOLUME = 200;
    float speed = glm::length(_lastVelocity);
    if ((speed > MIN_AUDIBLE_VELOCITY) && (speed < MAX_AUDIBLE_VELOCITY)) {
        for (int i = 0; i < numSamples; i++) {
            inputBuffer[i] += (int16_t) ((randFloat() - 0.5f) * VOLUME * speed) ;
        }
    }

    return;
}


#endif
