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

#include <NetworkPeer.h>
#include <HTTPConnection.h>
#include <HTTPManager.h>
#include <NLPacket.h>
#include <udt/Socket.h>

typedef QHash<QUuid, SharedNetworkPeer> NetworkPeerHash;

class IceServer : public QCoreApplication, public HTTPRequestHandler {
    Q_OBJECT
public:
    IceServer(int argc, char* argv[]);
    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false);
private slots:
    void clearInactivePeers();
private:
    bool packetVersionMatch(const udt::Packet& packet);
    void processPacket(std::unique_ptr<udt::Packet> packet);
    
    SharedNetworkPeer addOrUpdateHeartbeatingPeer(NLPacket& incomingPacket);
    void sendPeerInformationPacket(const NetworkPeer& peer, const HifiSockAddr* destinationSockAddr);

    QUuid _id;
    udt::Socket _serverSocket;
    NetworkPeerHash _activePeers;
    HTTPManager _httpManager;
};

#endif // hifi_IceServer_h
