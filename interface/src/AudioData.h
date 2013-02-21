//
//  AudioData.h
//  interface
//
//  Created by Stephen Birarda on 1/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __interface__AudioData__
#define __interface__AudioData__

#include <iostream>
#include <stdint.h>
#include "AudioRingBuffer.h"
#include "UDPSocket.h"

class AudioData {
    public:    
        AudioRingBuffer *ringBuffer;

        UDPSocket *audioSocket;
    
        int16_t *samplesToQueue;
    
        char *mixerAddress;
        unsigned short mixerPort;
    
        timeval lastCallback;
        float averagedLatency;
        float measuredJitter;
        float jitterBuffer;
        int wasStarved;
        
        float lastInputLoudness;
        float averagedInputLoudness;
    
        AudioData(int bufferLength);
        ~AudioData();
};

#endif /* defined(__interface__AudioData__) */
