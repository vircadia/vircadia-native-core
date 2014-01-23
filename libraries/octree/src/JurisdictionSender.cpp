//
//  JurisdictionSender.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded jurisdiction Sender for the Application
//

#include <PerfStat.h>

#include <OctalCode.h>
#include <SharedUtil.h>
#include <PacketHeaders.h>
#include "JurisdictionSender.h"


JurisdictionSender::JurisdictionSender(JurisdictionMap* map, NODE_TYPE type, PacketSenderNotify* notify) : 
    ReceivedPacketProcessor(),
    _jurisdictionMap(map),
    _packetSender(notify, JurisdictionSender::DEFAULT_PACKETS_PER_SECOND)
{
    _nodeType = type;
}

JurisdictionSender::~JurisdictionSender() {
}


void JurisdictionSender::processPacket(const HifiSockAddr& senderAddress, unsigned char*  packetData, ssize_t packetLength) {
    if (packetData[0] == PACKET_TYPE_JURISDICTION_REQUEST) {
        SharedNodePointer node = NodeList::getInstance()->nodeWithAddress(senderAddress);
        if (node) {
            QUuid nodeUUID = node->getUUID();
            lockRequestingNodes();
            _nodesRequestingJurisdictions.push(nodeUUID);
            unlockRequestingNodes();
        }
    }
}

bool JurisdictionSender::process() {
    bool continueProcessing = isStillRunning();

    // call our ReceivedPacketProcessor base class process so we'll get any pending packets
    if (continueProcessing && (continueProcessing = ReceivedPacketProcessor::process())) {
        // add our packet to our own queue, then let the PacketSender class do the rest of the work.
        static unsigned char buffer[MAX_PACKET_SIZE];
        unsigned char* bufferOut = &buffer[0];
        ssize_t sizeOut = 0;

        if (_jurisdictionMap) {
            sizeOut = _jurisdictionMap->packIntoMessage(bufferOut, MAX_PACKET_SIZE);
        } else {
            sizeOut = JurisdictionMap::packEmptyJurisdictionIntoMessage(getNodeType(), bufferOut, MAX_PACKET_SIZE);
        }
        int nodeCount = 0;

        lockRequestingNodes();
        while (!_nodesRequestingJurisdictions.empty()) {

            QUuid nodeUUID = _nodesRequestingJurisdictions.front();
            _nodesRequestingJurisdictions.pop();
            SharedNodePointer node = NodeList::getInstance()->nodeWithUUID(nodeUUID);

            if (node->getActiveSocket() != NULL) {
                const HifiSockAddr* nodeAddress = node->getActiveSocket();
                _packetSender.queuePacketForSending(*nodeAddress, bufferOut, sizeOut);
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
