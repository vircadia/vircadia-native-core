//
//  AudioData.cpp
//  interface
//
//  Created by Stephen Birarda on 1/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "AudioData.h"

AudioData::AudioData(int bufferLength) {
    sources = NULL;
    
    samplesToQueue = new int16_t[bufferLength / sizeof(int16_t)];
}

AudioData::AudioData(int numberOfSources, int bufferLength) {
    _numberOfSources = numberOfSources;
    
    sources = new AudioSource*[numberOfSources];
    
    for(int s = 0; s < numberOfSources; s++) {
        sources[s] = new AudioSource();
    }
    
    samplesToQueue = new int16_t[bufferLength / sizeof(int16_t)];
    averagedLatency = 0.0;
    lastCallback.tv_usec = 0;
    wasStarved = 0;
    measuredJitter = 0;
    jitterBuffer = 0;
    
}

AudioData::~AudioData() {
    if (sources != NULL) {
        for (int s = 0; s < _numberOfSources; s++) {
            delete sources[s];
        }
    }
    
    delete[] samplesToQueue;
    
    delete audioSocket;
}