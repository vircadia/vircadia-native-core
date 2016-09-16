//
//  ICEClient.h
//  tools/ice-client/src
//
//  Created by Seth Alves on 2016-9-16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_ICEClientApp_h
#define hifi_ICEClientApp_h

#include <QApplication>
#include <udt/Constants.h>
#include <udt/Socket.h>
#include <ReceivedMessage.h>
#include <NetworkPeer.h>


class ICEClientApp : public QCoreApplication {
    Q_OBJECT
public:
    ICEClientApp(int argc, char* argv[]);
    ~ICEClientApp();

private:
    void doSomething();
    void sendPacketToIceServer(PacketType packetType, const HifiSockAddr& iceServerSockAddr,
                               const QUuid& clientID, const QUuid& peerID);
    void icePingDomainServer();
    void processPacket(std::unique_ptr<udt::Packet> packet);

    bool _verbose;

    unsigned int _actionCount { 0 };
    unsigned int _actionMax { 0 };

    QUuid _sessionUUID;

    QTimer* _pingDomainTimer { nullptr };

    HifiSockAddr _iceServerAddr;

    HifiSockAddr _localSockAddr;
    HifiSockAddr _publicSockAddr;
    udt::Socket* _socket { nullptr };

    bool _domainServerPeerSet { false };
    NetworkPeer _domainServerPeer;

    // 0 -- time to talk to ice server
    // 1 -- waiting for ICEPingReply
    // 2 -- pause
    // 3 -- pause
    int _state { 0 };
};



#endif //hifi_ICEClientApp_h
