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
}


AudioData::~AudioData() {    
    delete audioSocket;
}

#endif
