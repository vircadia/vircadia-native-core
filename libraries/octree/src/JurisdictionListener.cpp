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
    _packetSender(notify, JurisdictionListener::DEFAULT_PACKETS_PER_SECOND)
{
    _nodeType = type;
    ReceivedPacketProcessor::_dontSleep = true; // we handle sleeping so this class doesn't need to
    
    connect(NodeList::getInstance(), &NodeList::nodeKilled, this, &JurisdictionListener::nodeKilled);
    //qDebug("JurisdictionListener::JurisdictionListener(NODE_TYPE type=%c)", type);
    
    // tell our NodeList we want to hear about nodes with our node type
    NodeList::getInstance()->addNodeTypeToInterestSet(type);
}

void JurisdictionListener::nodeKilled(SharedNodePointer node) {
    if (_jurisdictions.find(node->getUUID()) != _jurisdictions.end()) {
        _jurisdictions.erase(_jurisdictions.find(node->getUUID()));
    }
}

bool JurisdictionListener::queueJurisdictionRequest() {
    //qDebug() << "JurisdictionListener::queueJurisdictionRequest()";

    static unsigned char buffer[MAX_PACKET_SIZE];
    unsigned char* bufferOut = &buffer[0];
    ssize_t sizeOut = populateTypeAndVersion(bufferOut, PACKET_TYPE_JURISDICTION_REQUEST);
    int nodeCount = 0;

    NodeList* nodeList = NodeList::getInstance();
    
    foreach (const SharedNodePointer& node, nodeList->getNodeHash()) {
        if (nodeList->getNodeActiveSocketOrPing(node.data()) &&
            node->getType() == getNodeType()) {
            const HifiSockAddr* nodeAddress = node->getActiveSocket();
            _packetSender.queuePacketForSending(*nodeAddress, bufferOut, sizeOut);
            nodeCount++;
        }
    }

    if (nodeCount > 0){
        _packetSender.setPacketsPerSecond(nodeCount);
    } else {
        _packetSender.setPacketsPerSecond(NO_SERVER_CHECK_RATE);
    }
    
    // keep going if still running
    return isStillRunning();
}

void JurisdictionListener::processPacket(const HifiSockAddr& senderAddress, unsigned char*  packetData, ssize_t packetLength) {
    //qDebug() << "JurisdictionListener::processPacket()";
    if (packetData[0] == PACKET_TYPE_JURISDICTION) {
        SharedNodePointer node = NodeList::getInstance()->nodeWithAddress(senderAddress);
        if (node) {
            QUuid nodeUUID = node->getUUID();
            JurisdictionMap map;
            map.unpackFromMessage(packetData, packetLength);
            _jurisdictions[nodeUUID] = map;
        }
    }
}

bool JurisdictionListener::process() {
    //qDebug() << "JurisdictionListener::process()";
    bool continueProcessing = isStillRunning();

    // If we're still running, and we don't have any requests waiting to be sent, then queue our jurisdiction requests
    if (continueProcessing && !_packetSender.hasPacketsToSend()) {
        queueJurisdictionRequest();
    }
    
    if (continueProcessing) {
        //qDebug() << "JurisdictionListener::process() calling _packetSender.process()";
        continueProcessing = _packetSender.process();
    }
    if (continueProcessing) {
        // NOTE: This will sleep if there are no pending packets to process
        continueProcessing = ReceivedPacketProcessor::process();
    }
    
    return continueProcessing;
}
