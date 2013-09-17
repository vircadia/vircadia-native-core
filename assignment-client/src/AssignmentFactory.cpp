//
//  AssignmentFactory.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <PacketHeaders.h>

#include "Agent.h"
#include "audio/AudioMixer.h"
#include "avatars/AvatarMixer.h"

#include "AssignmentFactory.h"

Assignment* AssignmentFactory::unpackAssignment(const unsigned char* dataBuffer, int numBytes) {
    int headerBytes = numBytesForPacketHeader(dataBuffer);
    
    Assignment::Type type;
    memcpy(&type, dataBuffer + headerBytes, sizeof(type));
    
    switch (type) {
        case Assignment::AudioMixerType:
            return new AudioMixer(dataBuffer, numBytes);
        case Assignment::AvatarMixerType:
            return new AvatarMixer(dataBuffer, numBytes);
        case Assignment::AgentType:
            return new Agent(dataBuffer, numBytes);
        default:
            return new Assignment(dataBuffer, numBytes);
    }
}