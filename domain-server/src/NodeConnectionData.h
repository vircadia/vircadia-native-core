//
//  NodeConnectionData.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2015-08-24.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_NodeConnectionData_h
#define hifi_NodeConnectionData_h

#include <Node.h>

class NodeConnectionData {
public:
    static NodeConnectionData fromDataStream(QDataStream& dataStream, const SockAddr& senderSockAddr,
                                             bool isConnectRequest = true);
    
    QUuid connectUUID;
    quint64 lastPingTimestamp{ 0 }; // client-side send time of last connect/domain list request
    NodeType_t nodeType;
    SockAddr publicSockAddr;
    SockAddr localSockAddr;
    SockAddr senderSockAddr;
    QList<NodeType_t> interestList;
    QString placeName;
    QString hardwareAddress;
    QUuid machineFingerprint;
    QString SystemInfo;
    quint32 connectReason;
    quint64 previousConnectionUpTime;
    QByteArray protocolVersion;
};


#endif // hifi_NodeConnectionData_h
