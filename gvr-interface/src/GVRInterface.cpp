//
//  GVRInterface.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 11/18/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <NodeList.h>

#include "GVRInterface.h"

GVRInterface::GVRInterface(int argc, char* argv[]) : 
    QGuiApplication(argc, argv)
{
    NodeList* nodeList = NodeList::createInstance(NodeType::Agent);
    
    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, this, &GVRInterface::processDatagrams);
    
    QTimer* domainServerTimer = new QTimer(this);
    connect(domainServerTimer, &QTimer::timeout, nodeList, &NodeList::sendDomainServerCheckIn);
    domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_MSECS);
    
    QTimer* silentNodeRemovalTimer = new QTimer(this);
    connect(silentNodeRemovalTimer, &QTimer::timeout, nodeList, &NodeList::removeSilentNodes);
    silentNodeRemovalTimer->start(NODE_SILENCE_THRESHOLD_MSECS);
}

void GVRInterface::processDatagrams() {
    NodeList* nodeList = NodeList::getInstance();
    
    HifiSockAddr senderSockAddr;
    QByteArray incomingPacket;
    
    while (nodeList->getNodeSocket().hasPendingDatagrams()) {
        incomingPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(incomingPacket.data(), incomingPacket.size(),
            senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        qDebug() << "Processing a packet!";
        nodeList->processNodeData(senderSockAddr, incomingPacket);
    }
}