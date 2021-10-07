//
//  NodeConnectionData.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2015-08-24.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NodeConnectionData.h"

#include <QtCore/QDataStream>

NodeConnectionData NodeConnectionData::fromDataStream(QDataStream& dataStream, const SockAddr& senderSockAddr,
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

        // now the machine fingerprint
        dataStream >> newHeader.machineFingerprint;

        // and the operating system type
        QByteArray compressedSystemInfo;
        dataStream >> compressedSystemInfo;
        if (!compressedSystemInfo.isEmpty()) {
            newHeader.SystemInfo = qUncompress(compressedSystemInfo);
        }

        dataStream >> newHeader.connectReason;

        dataStream >> newHeader.previousConnectionUpTime;
    }

    dataStream >> newHeader.lastPingTimestamp;
    
    SocketType publicSocketType, localSocketType;
    dataStream >> newHeader.nodeType
        >> publicSocketType >> newHeader.publicSockAddr >> localSocketType >> newHeader.localSockAddr
        >> newHeader.interestList >> newHeader.placeName;
    newHeader.publicSockAddr.setType(publicSocketType);
    newHeader.localSockAddr.setType(localSocketType);

    // For WebRTC connections, the user client's signaling channel WebSocket address is used instead of the actual data 
    // channel's address.
    if (senderSockAddr.getType() == SocketType::WebRTC) {
        if (newHeader.publicSockAddr.getType() != SocketType::WebRTC
            || newHeader.localSockAddr.getType() != SocketType::WebRTC) {
            qDebug() << "Inconsistent WebRTC socket types!";
        }

        // We don't know whether it's a public or local connection so set both the same.
        auto address = senderSockAddr.getAddress();
        auto port = senderSockAddr.getPort();
        newHeader.publicSockAddr.setAddress(address);
        newHeader.publicSockAddr.setPort(port);
        newHeader.localSockAddr.setAddress(address);
        newHeader.localSockAddr.setPort(port);
    }

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
