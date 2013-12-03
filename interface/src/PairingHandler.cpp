//
//  PairingHandler.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/13/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#include <QtNetwork/QHostInfo>

#include <HifiSockAddr.h>
#include <NodeList.h>

#include "PairingHandler.h"

const char PAIRING_SERVER_HOSTNAME[] = "pairing.highfidelity.io";
const int PAIRING_SERVER_PORT = 7247;

PairingHandler* PairingHandler::getInstance() {
    static PairingHandler* instance = NULL;
    
    if (!instance) {
        instance = new PairingHandler();
    }
    
    return instance;
}

void PairingHandler::sendPairRequest() {
    
    // prepare the pairing request packet
    
    NodeList* nodeList = NodeList::getInstance();
    
    // use the getLocalAddress helper to get this client's listening address
    quint32 localAddress = htonl(getHostOrderLocalAddress());
    
    char pairPacket[24] = {};
    sprintf(pairPacket, "Find %d.%d.%d.%d:%hu",
            localAddress & 0xFF,
            (localAddress >> 8) & 0xFF,
            (localAddress >> 16) & 0xFF,
            (localAddress >> 24) & 0xFF,
            NodeList::getInstance()->getNodeSocket().localPort());
    
    qDebug("Sending pair packet: %s\n", pairPacket);
    
    HifiSockAddr pairingServerSocket(PAIRING_SERVER_HOSTNAME, PAIRING_SERVER_PORT);
    
    // send the pair request to the pairing server
    nodeList->getNodeSocket().writeDatagram((char*) pairPacket, strlen(pairPacket),
                                            pairingServerSocket.getAddress(), pairingServerSocket.getPort());
}
