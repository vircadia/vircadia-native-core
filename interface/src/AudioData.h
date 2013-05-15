//
//  AudioData.h
//  interface
//
//  Created by Stephen Birarda on 1/29/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __interface__AudioData__
#define __interface__AudioData__

#include <stdint.h>
#include <glm/glm.hpp>
#include "AudioRingBuffer.h"
#include "UDPSocket.h"
#include "Avatar.h"

class AudioData {
    public:
        AudioData();
        ~AudioData();
        AudioRingBuffer *ringBuffer;

        UDPSocket *audioSocket;
    
        Avatar *linkedAvatar;
    
        // store current mixer address and port
        in_addr_t mixerAddress;
        in_port_t mixerPort;
    
        timeval lastCallback;
        float averagedLatency;
        float measuredJitter;
        float jitterBuffer;
        int wasStarved;
        
        float lastInputLoudness;
    
        bool mixerLoopbackFlag;
    
        //  Added avatar acceleration and velocity for procedural effects sounds from client
        void setLastVelocity(glm::vec3 v) { _lastVelocity = v; };
        void setLastAcceleration(glm::vec3 a) { _lastAcceleration = a; };
        void addProceduralSounds(int16_t* inputBuffer, int numSamples);
        void analyzeEcho(int16_t* inputBuffer, int16_t* outputBuffer, int numSamples);
    
    private:
        glm::vec3 _lastVelocity;
        glm::vec3 _lastAcceleration;

    
};

#endif /* defined(__interface__AudioData__) */
