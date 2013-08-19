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
    _jurisdictionMap(map)
{
}

bool JurisdictionSender::process() {
//printf("JurisdictionSender::process() _packetsPerSecond=%d\n", _packetsPerSecond);
    // add our packet to our own queue, then let the PacketSender class do the rest of the work.
    if (_jurisdictionMap) {
//printf("JurisdictionSender::process() _jurisdictionMap=%p\n",_jurisdictionMap);
        unsigned char buffer[MAX_PACKET_SIZE];
        unsigned char* bufferOut = &buffer[0];
        ssize_t sizeOut = _jurisdictionMap->packIntoMessage(bufferOut, MAX_PACKET_SIZE);
        int nodeCount = 0;

        NodeList* nodeList = NodeList::getInstance();
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {

//printf("JurisdictionSender::process() node loop node=%d type=%c\n",node->getNodeID(), node->getType());

            // only send to the NodeTypes that are interested in our jurisdiction details
            const int numNodeTypes = 1; 
            const NODE_TYPE nodeTypes[numNodeTypes] = { NODE_TYPE_ANIMATION_SERVER };
            if (node->getActiveSocket() != NULL && memchr(nodeTypes, node->getType(), numNodeTypes)) {
                sockaddr* nodeAddress = node->getActiveSocket();
                queuePacket(*nodeAddress, bufferOut, sizeOut);
                nodeCount++;
            }
        }
        
        // set our packets per second to be the number of nodes
        setPacketsPerSecond(nodeCount);
        printf("loaded %d packets to send\n", nodeCount);
    }

    return PacketSender::process();
}
