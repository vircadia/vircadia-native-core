//
//  Client.cpp
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QThread>

#include <AddressManager.h>
#include <NodeList.h>
#include <PacketHeaders.h>

#include "Client.h"

Client::Client(QObject* parent) :
    QObject(parent)
{   
    // we need to make sure that required dependencies are created
    DependencyManager::set<AddressManager>();
    
    setupNetworking();
}

void Client::setupNetworking() {
    // setup the NodeList for this client
    auto nodeList = DependencyManager::set<NodeList>(NodeType::Agent, 0);
    
    // while datagram processing remains simple for targets using Client, we'll handle datagrams
    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, this, &Client::processDatagrams);
    
    // every second, ask the NodeList to check in with the domain server
    QTimer* domainCheckInTimer = new QTimer(this);
    domainCheckInTimer->setInterval(DOMAIN_SERVER_CHECK_IN_MSECS);
    connect(domainCheckInTimer, &QTimer::timeout, nodeList.data(), &NodeList::sendDomainServerCheckIn);
    
    // TODO: once the Client knows its Address on start-up we should be able to immediately send a check in here
    domainCheckInTimer->start();
    
    // handle the case where the domain stops talking to us
    // TODO: can we just have the nodelist do this when it sets up? Is there a user of the NodeList that wouldn't want this?
    connect(nodeList.data(), &NodeList::limitOfSilentDomainCheckInsReached, nodeList.data(), &NodeList::reset);
}

void Client::processDatagrams() {
    HifiSockAddr senderSockAddr;
    
    static QByteArray incomingPacket;
    
    auto nodeList = DependencyManager::get<NodeList>();
    
    while (DependencyManager::get<NodeList>()->getNodeSocket().hasPendingDatagrams()) {
        incomingPacket.resize(nodeList->getNodeSocket().pendingDatagramSize());
        nodeList->getNodeSocket().readDatagram(incomingPacket.data(), incomingPacket.size(),
                                               senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
    
        if (nodeList->packetVersionAndHashMatch(incomingPacket)) {
            
            PacketType incomingType = packetTypeForPacket(incomingPacket);
            // only process this packet if we have a match on the packet version
            switch (incomingType) {
                default:
                    nodeList->processNodeData(senderSockAddr, incomingPacket);
                    break;
            }
        }
    }
}