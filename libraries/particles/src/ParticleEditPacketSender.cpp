//
//  ParticleEditPacketSender.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel packet Sender for the Application
//

#include <assert.h>
#include <PerfStat.h>
#include <OctalCode.h>
#include <PacketHeaders.h>
#include "ParticleEditPacketSender.h"
#include "Particle.h"


void ParticleEditPacketSender::sendEditParticleMessage(PACKET_TYPE type, const ParticleDetail& detail) {
    // allows app to disable sending if for example voxels have been disabled
    if (!_shouldSend) {
        return; // bail early
    }

    static unsigned char bufferOut[MAX_PACKET_SIZE];
    int sizeOut = 0;

    // This encodes the voxel edit message into a buffer...
    if (Particle::encodeParticleEditMessageDetails(type, 1, &detail, &bufferOut[0], _maxPacketSize, sizeOut)){
        // If we don't have voxel jurisdictions, then we will simply queue up these packets and wait till we have
        // jurisdictions for processing
        if (!serversExist()) {
            queuePendingPacketToNodes(type, bufferOut, sizeOut);
        } else {
            queuePacketToNodes(bufferOut, sizeOut);
        }
    }
}

void ParticleEditPacketSender::queueParticleEditMessages(PACKET_TYPE type, int numberOfDetails, ParticleDetail* details) {
    if (!_shouldSend) {
        return; // bail early
    }

    for (int i = 0; i < numberOfDetails; i++) {
        // use MAX_PACKET_SIZE since it's static and guaranteed to be larger than _maxPacketSize
        static unsigned char bufferOut[MAX_PACKET_SIZE]; 
        int sizeOut = 0;
        
        if (Particle::encodeParticleEditMessageDetails(type, 1, &details[i], &bufferOut[0], _maxPacketSize, sizeOut)) {
            queueOctreeEditMessage(type, bufferOut, sizeOut);
        }
    }    
}

