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
#include "messages/MessagesMixer.h"

ThreadedAssignment* AssignmentFactory::unpackAssignment(ReceivedMessage& message) {

    quint8 packedType;
    message.peekPrimitive(&packedType);

    Assignment::Type unpackedType = (Assignment::Type) packedType;

    switch (unpackedType) {
        case Assignment::AudioMixerType:
            return new AudioMixer(message);
        case Assignment::AvatarMixerType:
            return new AvatarMixer(message);
        case Assignment::AgentType:
            return new Agent(message);
        case Assignment::EntityServerType:
            return new EntityServer(message);
        case Assignment::AssetServerType:
            return new AssetServer(message);
        case Assignment::MessagesMixerType:
            return new MessagesMixer(message);
        default:
            return NULL;
    }
}
