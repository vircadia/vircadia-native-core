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
    
    dataStream >> newHeader.nodeType
        >> newHeader.publicSockAddr >> newHeader.localSockAddr
        >> newHeader.interestList >> newHeader.placeName;

    // A WebRTC web client doesn't necessarily know it's public Internet or local network addresses, and for WebRTC they aren't
    // needed: for WebRTC, the data channel ID is the important thing. The client's public Internet IP address still needs to
    // be known for domain access permissions, though, and this can be obtained from the WebSocket signaling connection.
    if (senderSockAddr.getSocketType() == SocketType::WebRTC) {
        // WEBRTC TODO: Rather than setting the SocketType here, serialize and deserialize the SocketType in the leading byte of
        // the 5 bytes used to encode the IP address.
        newHeader.publicSockAddr.setSocketType(SocketType::WebRTC);
        newHeader.localSockAddr.setSocketType(SocketType::WebRTC);

        // WEBRTC TODO: Set the public Internet address obtained from the WebSocket used in WebRTC signaling.

        newHeader.publicSockAddr.setPort(senderSockAddr.getPort());  // We don't know whether it's a public or local connection
        newHeader.localSockAddr.setPort(senderSockAddr.getPort());   // so set both ports.
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
