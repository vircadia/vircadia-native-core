//
//  AssignmentFactory.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PacketHeaders.h>

#include "Agent.h"
#include "AssignmentFactory.h"
#include "audio/AudioMixer.h"
#include "avatars/AvatarMixer.h"
#include "metavoxels/MetavoxelServer.h"
#include "entities/EntityServer.h"
#include "particles/ParticleServer.h"
#include "voxels/VoxelServer.h"

ThreadedAssignment* AssignmentFactory::unpackAssignment(const QByteArray& packet) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));

    quint8 packedType;
    packetStream >> packedType;    
    
    Assignment::Type unpackedType = (Assignment::Type) packedType;
    
    switch (unpackedType) {
        case Assignment::AudioMixerType:
            return new AudioMixer(packet);
        case Assignment::AvatarMixerType:
            return new AvatarMixer(packet);
        case Assignment::AgentType:
            return new Agent(packet);
        case Assignment::VoxelServerType:
            return new VoxelServer(packet);
        case Assignment::ParticleServerType:
            return new ParticleServer(packet);
        case Assignment::MetavoxelServerType:
            return new MetavoxelServer(packet);
        case Assignment::EntityServerType:
            return new EntityServer(packet);
        default:
            return NULL;
    }
}
