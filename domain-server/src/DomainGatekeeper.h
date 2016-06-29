//
//  DomainGatekeeper.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2015-08-24.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_DomainGatekeeper_h
#define hifi_DomainGatekeeper_h

#include <unordered_map>

#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>

#include <DomainHandler.h>

#include <NLPacket.h>
#include <Node.h>
#include <UUIDHasher.h>

#include "NodeConnectionData.h"
#include "PendingAssignedNodeData.h"

class DomainServer;

class DomainGatekeeper : public QObject {
    Q_OBJECT
public:
    DomainGatekeeper(DomainServer* server);
    
    void addPendingAssignedNode(const QUuid& nodeUUID, const QUuid& assignmentUUID,
                                const QUuid& walletUUID, const QString& nodeVersion);
    QUuid assignmentUUIDForPendingAssignment(const QUuid& tempUUID);
    
    void preloadAllowedUserPublicKeys();
    
    void removeICEPeer(const QUuid& peerUUID) { _icePeers.remove(peerUUID); }

    static void sendProtocolMismatchConnectionDenial(const HifiSockAddr& senderSockAddr);
public slots:
    void processConnectRequestPacket(QSharedPointer<ReceivedMessage> message);
    void processICEPingPacket(QSharedPointer<ReceivedMessage> message);
    void processICEPingReplyPacket(QSharedPointer<ReceivedMessage> message);
    void processICEPeerInformationPacket(QSharedPointer<ReceivedMessage> message);

    void publicKeyJSONCallback(QNetworkReply& requestReply);
    
signals:
    void killNode(SharedNodePointer node);
    void connectedNode(SharedNodePointer node);

public slots:
    void updateNodePermissions();

private slots:
    void handlePeerPingTimeout();
private:
    SharedNodePointer processAssignmentConnectRequest(const NodeConnectionData& nodeConnection,
                                                      const PendingAssignedNodeData& pendingAssignment);
    SharedNodePointer processAgentConnectRequest(const NodeConnectionData& nodeConnection,
                                                 const QString& username,
                                                 const QByteArray& usernameSignature);
    SharedNodePointer addVerifiedNodeFromConnectRequest(const NodeConnectionData& nodeConnection,
                                                        QUuid nodeID = QUuid());
    
    bool verifyUserSignature(const QString& username, const QByteArray& usernameSignature,
                             const HifiSockAddr& senderSockAddr);
    bool isWithinMaxCapacity();
    
    bool shouldAllowConnectionFromNode(const QString& username, const QByteArray& usernameSignature,
                                       const HifiSockAddr& senderSockAddr);
    
    void sendConnectionTokenPacket(const QString& username, const HifiSockAddr& senderSockAddr);
    static void sendConnectionDeniedPacket(const QString& reason, const HifiSockAddr& senderSockAddr,
            DomainHandler::ConnectionRefusedReason reasonCode = DomainHandler::ConnectionRefusedReason::Unknown);
    
    void pingPunchForConnectingPeer(const SharedNetworkPeer& peer);
    
    void requestUserPublicKey(const QString& username);
    
    DomainServer* _server;
    
    std::unordered_map<QUuid, PendingAssignedNodeData> _pendingAssignedNodes;
    
    QHash<QUuid, SharedNetworkPeer> _icePeers;
    
    QHash<QString, QUuid> _connectionTokenHash;
    QHash<QString, QByteArray> _userPublicKeys;
};


#endif // hifi_DomainGatekeeper_h
