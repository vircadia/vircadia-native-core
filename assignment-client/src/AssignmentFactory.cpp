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

#include <udt/PacketHeaders.h>

#include "Agent.h"
#include "AssignmentFactory.h"
#include "audio/AudioMixer.h"
#include "avatars/AvatarMixer.h"
#include "entities/EntityServer.h"
#include "assets/AssetServer.h"

ThreadedAssignment* AssignmentFactory::unpackAssignment(NLPacket& packet) {

    quint8 packedType;
    packet.peekPrimitive(&packedType);

    Assignment::Type unpackedType = (Assignment::Type) packedType;

    switch (unpackedType) {
        case Assignment::AudioMixerType:
            return new AudioMixer(packet);
        case Assignment::AvatarMixerType:
            return new AvatarMixer(packet);
        case Assignment::AgentType:
            return new Agent(packet);
        case Assignment::EntityServerType:
            return new EntityServer(packet);
        case Assignment::AssetServerType:
            return new AssetServer(packet);
        default:
            return NULL;
    }
}
