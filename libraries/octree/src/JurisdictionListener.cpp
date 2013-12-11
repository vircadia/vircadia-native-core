//
//  JurisdictionListener.cpp
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
#include "JurisdictionListener.h"


JurisdictionListener::JurisdictionListener(NODE_TYPE type, PacketSenderNotify* notify) : 
    PacketSender(notify, JurisdictionListener::DEFAULT_PACKETS_PER_SECOND)
{
    _nodeType = type;
    ReceivedPacketProcessor::_dontSleep = true; // we handle sleeping so this class doesn't need to
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addHook(this);

    //qDebug("JurisdictionListener::JurisdictionListener(NODE_TYPE type=%c)\n", type);

}

JurisdictionListener::~JurisdictionListener() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->removeHook(this);
}

void JurisdictionListener::nodeAdded(Node* node) {
    // nothing to do. But need to implement it.
}

void JurisdictionListener::nodeKilled(Node* node) {
    if (_jurisdictions.find(node->getUUID()) != _jurisdictions.end()) {
        _jurisdictions.erase(_jurisdictions.find(node->getUUID()));
    }
}

bool JurisdictionListener::queueJurisdictionRequest() {
    //qDebug() << "JurisdictionListener::queueJurisdictionRequest()\n";

    static unsigned char buffer[MAX_PACKET_SIZE];
    unsigned char* bufferOut = &buffer[0];
    ssize_t sizeOut = populateTypeAndVersion(bufferOut, PACKET_TYPE_JURISDICTION_REQUEST);
    int nodeCount = 0;

    NodeList* nodeList = NodeList::getInstance();
    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
        if (nodeList->getNodeActiveSocketOrPing(&(*node)) && 
            node->getType() == getNodeType()) {
            const HifiSockAddr* nodeAddress = node->getActiveSocket();
            PacketSender::queuePacketForSending(*nodeAddress, bufferOut, sizeOut);
            nodeCount++;
        }
    }

    if (nodeCount > 0){
        setPacketsPerSecond(nodeCount);
    } else {
        setPacketsPerSecond(NO_SERVER_CHECK_RATE);
    }
    
    // keep going if still running
    return isStillRunning();
}

void JurisdictionListener::processPacket(const HifiSockAddr& senderAddress, unsigned char*  packetData, ssize_t packetLength) {
    if (packetData[0] == PACKET_TYPE_JURISDICTION) {
        Node* node = NodeList::getInstance()->nodeWithAddress(senderAddress);
        if (node) {
            QUuid nodeUUID = node->getUUID();
            JurisdictionMap map;
            map.unpackFromMessage(packetData, packetLength);
            _jurisdictions[nodeUUID] = map;
        }
    }
}

bool JurisdictionListener::process() {
    bool continueProcessing = isStillRunning();

    // If we're still running, and we don't have any requests waiting to be sent, then queue our jurisdiction requests
    if (continueProcessing && !hasPacketsToSend()) {
        queueJurisdictionRequest();
        continueProcessing = PacketSender::process();
    }
    if (continueProcessing) {
        // NOTE: This will sleep if there are no pending packets to process
        continueProcessing = ReceivedPacketProcessor::process();
    }
    
    return continueProcessing;
}
