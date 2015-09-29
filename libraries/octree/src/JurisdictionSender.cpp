//
//  JurisdictionSender.cpp
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PerfStat.h>

#include <OctalCode.h>
#include <SharedUtil.h>
#include <udt/PacketHeaders.h>
#include "JurisdictionSender.h"


JurisdictionSender::JurisdictionSender(JurisdictionMap* map, NodeType_t type) :
    ReceivedPacketProcessor(),
    _jurisdictionMap(map),
    _nodeType(type),
    _packetSender(JurisdictionSender::DEFAULT_PACKETS_PER_SECOND)
{
}

JurisdictionSender::~JurisdictionSender() {
}

void JurisdictionSender::processPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    if (message->getType() == PacketType::JurisdictionRequest) {
        lockRequestingNodes();
        _nodesRequestingJurisdictions.push(sendingNode->getUUID());
        unlockRequestingNodes();
    }
}

bool JurisdictionSender::process() {
    bool continueProcessing = isStillRunning();

    // call our ReceivedPacketProcessor base class process so we'll get any pending packets
    if (continueProcessing && (continueProcessing = ReceivedPacketProcessor::process())) {
        auto packet = (_jurisdictionMap) ? _jurisdictionMap->packIntoPacket()
                                         : JurisdictionMap::packEmptyJurisdictionIntoMessage(getNodeType());
        int nodeCount = 0;

        lockRequestingNodes();
        while (!_nodesRequestingJurisdictions.empty()) {

            QUuid nodeUUID = _nodesRequestingJurisdictions.front();
            _nodesRequestingJurisdictions.pop();
            SharedNodePointer node = DependencyManager::get<NodeList>()->nodeWithUUID(nodeUUID);

            if (node && node->getActiveSocket()) {
                _packetSender.queuePacketForSending(node, std::move(packet));
                nodeCount++;
            }
        }
        unlockRequestingNodes();

        // set our packets per second to be the number of nodes
        _packetSender.setPacketsPerSecond(nodeCount);

        continueProcessing = _packetSender.process();
    }
    return continueProcessing;
}
