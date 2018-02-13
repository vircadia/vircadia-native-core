//
//  ICEClientApp.h
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

#include <QCoreApplication>
#include <udt/Constants.h>
#include <udt/Socket.h>
#include <ReceivedMessage.h>
#include <NetworkPeer.h>


class ICEClientApp : public QCoreApplication {
    Q_OBJECT
public:
    ICEClientApp(int argc, char* argv[]);
    ~ICEClientApp();

    const int stunFailureExitStatus { 1 };
    const int iceFailureExitStatus { 2 };
    const int domainPingExitStatus { 3 };

    const int stunResponseTimeoutMilliSeconds { 2000 };
    const int iceResponseTimeoutMilliSeconds { 2000 };

public slots:
    void iceResponseTimeout();
    void stunResponseTimeout();

private:
    enum State {
        lookUpStunServer, // 0
        sendStunRequestPacket, // 1
        waitForStunResponse, // 2
        talkToIceServer, // 3
        waitForIceReply, // 4
        pause0, // 5
        pause1 // 6
    };

    void closeSocket();
    void openSocket();

    void setState(int newState);

    void doSomething();
    void sendPacketToIceServer(PacketType packetType, const HifiSockAddr& iceServerSockAddr,
                               const QUuid& clientID, const QUuid& peerID);
    void icePingDomainServer();
    void processSTUNResponse(std::unique_ptr<udt::BasePacket> packet);
    void processPacket(std::unique_ptr<udt::Packet> packet);
    void checkDomainPingCount();

    bool _verbose;
    bool _cacheSTUNResult; // should we only talk to stun server once?
    bool _stunResultSet { false }; // have we already talked to stun server?

    HifiSockAddr _stunSockAddr;

    unsigned int _actionCount { 0 };
    unsigned int _actionMax { 0 };

    QUuid _sessionUUID;
    QUuid _domainID;

    QTimer* _pingDomainTimer { nullptr };

    HifiSockAddr _iceServerAddr;

    HifiSockAddr _localSockAddr;
    HifiSockAddr _publicSockAddr;
    udt::Socket* _socket { nullptr };

    bool _domainServerPeerSet { false };
    NetworkPeer _domainServerPeer;

    int _state { 0 };

    QTimer _stunResponseTimer;
    QTimer _iceResponseTimer;
    int _domainPingCount { 0 };
};





#endif //hifi_ICEClientApp_h
