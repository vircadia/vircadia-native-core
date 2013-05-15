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
    const float MAX_AUDIBLE_VELOCITY = 6.0;
    const float MIN_AUDIBLE_VELOCITY = 0.1;
    float speed = glm::length(_lastVelocity);
    float volume = 400 * (1.f - speed/MAX_AUDIBLE_VELOCITY);
    //  Add a noise-modulated sinewave with volume that tapers off with speed increasing
    if ((speed > MIN_AUDIBLE_VELOCITY) && (speed < MAX_AUDIBLE_VELOCITY)) {
        for (int i = 0; i < numSamples; i++) {
            inputBuffer[i] += (int16_t) ((cosf((float)i / 8.f * speed) * randFloat()) * volume * speed) ;
        }
    }

    return;
}

void AudioData::analyzeEcho(int16_t* inputBuffer, int16_t* outputBuffer, int numSamples) {
    //  Compare output and input streams, looking for evidence of correlation needing echo cancellation
    //  
    //  OFFSET_RANGE tells us how many samples to vary the analysis window when looking for correlation,
    //  and should be equal to the largest physical distance between speaker and microphone, where
    //  OFFSET_RANGE =  1 / (speedOfSound (meters / sec) / SamplingRate (samples / sec)) * distance
    //
    const int OFFSET_RANGE = 10;
    const int SIGNAL_FLOOR = 1000;
    float correlation[2 * OFFSET_RANGE + 1];
    int numChecked = 0;
    bool foundSignal = false;
    for (int offset = -OFFSET_RANGE; offset <= OFFSET_RANGE; offset++) {
        for (int i = 0; i < numSamples; i++) {
            if ((i + offset >= 0) && (i + offset < numSamples)) {
                correlation[offset + OFFSET_RANGE] +=
                            (float) abs(inputBuffer[i] - outputBuffer[i + offset]);
                numChecked++;
                foundSignal |= (inputBuffer[i] > SIGNAL_FLOOR);
            }
        }
        correlation[offset + OFFSET_RANGE] /= numChecked;
        numChecked = 0;
        if (foundSignal) {
            printLog("%4.2f, ", correlation[offset + OFFSET_RANGE]);
        }
    }
    if (foundSignal) printLog("\n");
}



#endif
