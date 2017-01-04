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
    void publicKeyJSONErrorCallback(QNetworkReply& requestReply);

    void getIsGroupMemberJSONCallback(QNetworkReply& requestReply);
    void getIsGroupMemberErrorCallback(QNetworkReply& requestReply);

    void getDomainOwnerFriendsListJSONCallback(QNetworkReply& requestReply);
    void getDomainOwnerFriendsListErrorCallback(QNetworkReply& requestReply);

    void refreshGroupsCache();

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
            DomainHandler::ConnectionRefusedReason reasonCode = DomainHandler::ConnectionRefusedReason::Unknown,
            QString extraInfo = QString());
    
    void pingPunchForConnectingPeer(const SharedNetworkPeer& peer);
    
    void requestUserPublicKey(const QString& username);
    
    DomainServer* _server;
    
    std::unordered_map<QUuid, PendingAssignedNodeData> _pendingAssignedNodes;
    
    QHash<QUuid, SharedNetworkPeer> _icePeers;
    
    QHash<QString, QUuid> _connectionTokenHash;
    QHash<QString, QByteArray> _userPublicKeys;
    QSet<QString> _inFlightPublicKeyRequests; // keep track of which we've already asked for
    QSet<QString> _domainOwnerFriends; // keep track of friends of the domain owner
    QSet<QString> _inFlightGroupMembershipsRequests; // keep track of which we've already asked for

    NodePermissions setPermissionsForUser(bool isLocalUser, QString verifiedUsername, const QHostAddress& senderAddress, 
                                          const QString& hardwareAddress, const QUuid& machineFingerprint);

    void getGroupMemberships(const QString& username);
    // void getIsGroupMember(const QString& username, const QUuid groupID);
    void getDomainOwnerFriendsList();
};


#endif // hifi_DomainGatekeeper_h
