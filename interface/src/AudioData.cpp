//
//  AudioData.cpp
//  interface
//
//  Created by Stephen Birarda on 1/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "AudioData.h"

AudioData::AudioData(int bufferLength) {
    mixerAddress = NULL;
    mixerPort = 0;
    
    samplesToQueue = new int16_t[bufferLength / sizeof(int16_t)];
    averagedLatency = 0.0;
    lastCallback.tv_usec = 0;
    wasStarved = 0;
    measuredJitter = 0;
    jitterBuffer = 0;
}


AudioData::~AudioData() {    
    delete[] samplesToQueue;
    delete audioSocket;
}