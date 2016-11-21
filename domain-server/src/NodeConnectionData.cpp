//
//  NodeConnectionData.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2015-08-24.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NodeConnectionData.h"

#include <QtCore/QDataStream>

NodeConnectionData NodeConnectionData::fromDataStream(QDataStream& dataStream, const HifiSockAddr& senderSockAddr,
                                                      bool isConnectRequest) {
    NodeConnectionData newHeader;
    
    if (isConnectRequest) {
        dataStream >> newHeader.connectUUID;

        // Read out the protocol version signature from the connect message
        char* rawBytes;
        uint length;

        dataStream.readBytes(rawBytes, length);
        newHeader.protocolVersion = QByteArray(rawBytes, length);

        // NOTE: QDataStream::readBytes() - The buffer is allocated using new []. Destroy it with the delete [] operator.
        delete[] rawBytes;

        // read the hardware address sent by the client
        dataStream >> newHeader.hardwareAddress;
    }
    
    dataStream >> newHeader.nodeType
        >> newHeader.publicSockAddr >> newHeader.localSockAddr
        >> newHeader.interestList >> newHeader.placeName;

    newHeader.senderSockAddr = senderSockAddr;
    
    if (newHeader.publicSockAddr.getAddress().isNull()) {
        // this node wants to use us its STUN server
        // so set the node public address to whatever we perceive the public address to be
        
        // if the sender is on our box then leave its public address to 0 so that
        // other users attempt to reach it on the same address they have for the domain-server
        if (senderSockAddr.getAddress().isLoopback()) {
            newHeader.publicSockAddr.setAddress(QHostAddress());
        } else {
            newHeader.publicSockAddr.setAddress(senderSockAddr.getAddress());
        }
    }
    
    return newHeader;
}
