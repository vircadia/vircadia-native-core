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


JurisdictionSender::JurisdictionSender(JurisdictionMap* map, PacketSenderNotify* notify) : 
    PacketSender(notify, JurisdictionSender::DEFAULT_PACKETS_PER_SECOND), 
    ReceivedPacketProcessor(),
    _jurisdictionMap(map)
{
    pthread_mutex_init(&_requestingNodeMutex, 0);
}

JurisdictionSender::~JurisdictionSender() {
    pthread_mutex_destroy(&_requestingNodeMutex);
}


void JurisdictionSender::processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength) {
    if (packetData[0] == PACKET_TYPE_VOXEL_JURISDICTION_REQUEST) {
        Node* node = NodeList::getInstance()->nodeWithAddress(&senderAddress);
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
            sizeOut = JurisdictionMap::packEmptyJurisdictionIntoMessage(bufferOut, MAX_PACKET_SIZE);
        }
        int nodeCount = 0;

        lockRequestingNodes();
        while (!_nodesRequestingJurisdictions.empty()) {

            QUuid nodeUUID = _nodesRequestingJurisdictions.front();
            _nodesRequestingJurisdictions.pop();
            Node* node = NodeList::getInstance()->nodeWithUUID(nodeUUID);

            if (node->getActiveSocket() != NULL) {
                sockaddr* nodeAddress = node->getActiveSocket();
                queuePacketForSending(*nodeAddress, bufferOut, sizeOut);
                nodeCount++;
            }
        }
        unlockRequestingNodes();

        // set our packets per second to be the number of nodes
        setPacketsPerSecond(nodeCount);

        continueProcessing = PacketSender::process();
    }
    return continueProcessing;
}
