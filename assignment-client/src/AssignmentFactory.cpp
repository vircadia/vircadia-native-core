//
//  AssignmentFactory.cpp
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <PacketHeaders.h>

#include <ParticleServer.h>

#include <VoxelServer.h>

#include "Agent.h"
#include "AssignmentFactory.h"
#include "audio/AudioMixer.h"
#include "avatars/AvatarMixer.h"
#include "metavoxels/MetavoxelServer.h"

ThreadedAssignment* AssignmentFactory::unpackAssignment(const unsigned char* dataBuffer, int numBytes) {
    int headerBytes = numBytesForPacketHeader(dataBuffer);
    
    Assignment::Type assignmentType = Assignment::AllTypes;
    memcpy(&assignmentType, dataBuffer + headerBytes, sizeof(Assignment::Type));
    
    switch (assignmentType) {
        case Assignment::AudioMixerType:
            return new AudioMixer(dataBuffer, numBytes);
        case Assignment::AvatarMixerType:
            return new AvatarMixer(dataBuffer, numBytes);
        case Assignment::AgentType:
            return new Agent(dataBuffer, numBytes);
        case Assignment::VoxelServerType:
            return new VoxelServer(dataBuffer, numBytes);
        case Assignment::ParticleServerType:
            return new ParticleServer(dataBuffer, numBytes);
        case Assignment::MetavoxelServerType:
            return new MetavoxelServer(dataBuffer, numBytes);
        default:
            return NULL;
    }
}
