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

#include "Client.h"

#include <AccountManager.h>
#include <AddressManager.h>
#include <HifiSockAddr.h>
#include <NodeList.h>
#include <PacketHeaders.h>

Client::Client(QObject* parent) :
    QObject(parent)
{   
    // we need to make sure that required dependencies are created
    DependencyManager::set<AddressManager>();
    
    setupNetworking();
}

void Client::setupNetworking() {
    // once Application order of instantiation is fixed this should be done from AccountManager
    AccountManager::getInstance().setAuthURL(DEFAULT_NODE_AUTH_URL);
    
    // setup the NodeList for this client
    DependencyManager::registerInheritance<LimitedNodeList, NodeList>(); 
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

void Client::processVerifiedPacket(const HifiSockAddr& senderSockAddr, const QByteArray& incomingPacket) {
    DependencyManager::get<NodeList>()->processNodeData(senderSockAddr, incomingPacket);
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
            processVerifiedPacket(senderSockAddr, incomingPacket);
        }
    }
}
