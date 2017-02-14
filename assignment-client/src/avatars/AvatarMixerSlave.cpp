//
//  AvatarMixerSlave.cpp
//  assignment-client/src/avatar
//
//  Created by Brad Hefta-Gaub on 2/14/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <LogHandler.h>
#include <NetworkAccessManager.h>
#include <NodeList.h>
#include <Node.h>
#include <OctreeConstants.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <StDev.h>
#include <UUID.h>

#include "AvatarMixer.h"
#include "AvatarMixerClientData.h"
#include "AvatarMixerSlave.h"


void AvatarMixerSlave::configure(ConstIter begin, ConstIter end) {
    _begin = begin;
    _end = end;
}

void AvatarMixerSlave::processIncomingPackets(const SharedNodePointer& node) {
    auto nodeData = dynamic_cast<AvatarMixerClientData*>(node->getLinkedData());
    if (nodeData) {
        nodeData->processQueuedAvatarDataPackets();
    }
}

