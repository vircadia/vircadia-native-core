//
//  IceServer.h
//  ice-server/src
//
//  Created by Stephen Birarda on 2014-10-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_IceServer_h
#define hifi_IceServer_h

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>
#include <QUdpSocket>

#include <openssl/rsa.h>

#include <UUIDHasher.h>

#include <NetworkPeer.h>
#include <HTTPConnection.h>
#include <HTTPManager.h>
#include <NLPacket.h>
#include <udt/Socket.h>

class QNetworkReply;

class IceServer : public QCoreApplication {
    Q_OBJECT
public:
    IceServer(int argc, char* argv[]);
private slots:
    void clearInactivePeers();
    void publicKeyReplyFinished(QNetworkReply* reply);
private:
    bool packetVersionMatch(const udt::Packet& packet);
    void processPacket(std::unique_ptr<udt::Packet> packet);
    
    SharedNetworkPeer addOrUpdateHeartbeatingPeer(NLPacket& incomingPacket);
    void sendPeerInformationPacket(const NetworkPeer& peer, const HifiSockAddr* destinationSockAddr);

    bool isVerifiedHeartbeat(const QUuid& domainID, const QByteArray& plaintext, const QByteArray& signature);
    void requestDomainPublicKey(const QUuid& domainID);

    QUuid _id;
    udt::Socket _serverSocket;

    using NetworkPeerHash = QHash<QUuid, SharedNetworkPeer>;
    NetworkPeerHash _activePeers;

    using RSAUniquePtr = std::unique_ptr<RSA, std::function<void(RSA*)>>;
    using DomainPublicKeyHash = std::unordered_map<QUuid, RSAUniquePtr>;
    DomainPublicKeyHash _domainPublicKeys;

    QSet<QUuid> _pendingPublicKeyRequests;
};

#endif // hifi_IceServer_h
