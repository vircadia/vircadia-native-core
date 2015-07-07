//
//  IceServer.cpp
//  ice-server/src
//
//  Created by Stephen Birarda on 2014-10-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTimer>

#include <LimitedNodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "IceServer.h"

const int CLEAR_INACTIVE_PEERS_INTERVAL_MSECS = 1 * 1000;
const int PEER_SILENCE_THRESHOLD_MSECS = 5 * 1000;

const quint16 ICE_SERVER_MONITORING_PORT = 40110;

IceServer::IceServer(int argc, char* argv[]) :
	QCoreApplication(argc, argv),
    _id(QUuid::createUuid()),
    _serverSocket(),
    _activePeers(),
    _httpManager(ICE_SERVER_MONITORING_PORT, QString("%1/web/").arg(QCoreApplication::applicationDirPath()), this)
{
    // start the ice-server socket
    qDebug() << "ice-server socket is listening on" << ICE_SERVER_DEFAULT_PORT;
    qDebug() << "monitoring http endpoint is listening on " << ICE_SERVER_MONITORING_PORT;
    _serverSocket.bind(QHostAddress::AnyIPv4, ICE_SERVER_DEFAULT_PORT);

    // call our process datagrams slot when the UDP socket has packets ready
    connect(&_serverSocket, &QUdpSocket::readyRead, this, &IceServer::processDatagrams);

    // setup our timer to clear inactive peers
    QTimer* inactivePeerTimer = new QTimer(this);
    connect(inactivePeerTimer, &QTimer::timeout, this, &IceServer::clearInactivePeers);
    inactivePeerTimer->start(CLEAR_INACTIVE_PEERS_INTERVAL_MSECS);

}

void IceServer::processDatagrams() {
    HifiSockAddr sendingSockAddr;
    QByteArray incomingPacket;

    while (_serverSocket.hasPendingDatagrams()) {
        incomingPacket.resize(_serverSocket.pendingDatagramSize());

        _serverSocket.readDatagram(incomingPacket.data(), incomingPacket.size(),
                                   sendingSockAddr.getAddressPointer(), sendingSockAddr.getPortPointer());

        PacketType::Value packetType = packetTypeForPacket(incomingPacket);

        if (packetType == PacketTypeIceServerHeartbeat) {
            SharedNetworkPeer peer = addOrUpdateHeartbeatingPeer(incomingPacket);

            // so that we can send packets to the heartbeating peer when we need, we need to activate a socket now
            peer->activateMatchingOrNewSymmetricSocket(sendingSockAddr);
        } else if (packetType == PacketTypeIceServerQuery) {

            // this is a node hoping to connect to a heartbeating peer - do we have the heartbeating peer?
            QUuid senderUUID = uuidFromPacketHeader(incomingPacket);

            // pull the public and private sock addrs for this peer
            HifiSockAddr publicSocket, localSocket;

            QDataStream hearbeatStream(incomingPacket);
            hearbeatStream.skipRawData(numBytesForPacketHeader(incomingPacket));

            hearbeatStream >> publicSocket >> localSocket;

            // check if this node also included a UUID that they would like to connect to
            QUuid connectRequestID;
            hearbeatStream >> connectRequestID;

            SharedNetworkPeer matchingPeer = _activePeers.value(connectRequestID);

            if (matchingPeer) {
                // we have the peer they want to connect to - send them pack the information for that peer
                sendPeerInformationPacket(*(matchingPeer.data()), &sendingSockAddr);

                // we also need to send them to the active peer they are hoping to connect to
                // create a dummy peer object we can pass to sendPeerInformationPacket

                NetworkPeer dummyPeer(senderUUID, publicSocket, localSocket);
                sendPeerInformationPacket(dummyPeer, matchingPeer->getActiveSocket());
            }
        }
    }
}

SharedNetworkPeer IceServer::addOrUpdateHeartbeatingPeer(const QByteArray& incomingPacket) {
    QUuid senderUUID = uuidFromPacketHeader(incomingPacket);

    // pull the public and private sock addrs for this peer
    HifiSockAddr publicSocket, localSocket;

    QDataStream hearbeatStream(incomingPacket);
    hearbeatStream.skipRawData(numBytesForPacketHeader(incomingPacket));

    hearbeatStream >> publicSocket >> localSocket;

    // make sure we have this sender in our peer hash
    SharedNetworkPeer matchingPeer = _activePeers.value(senderUUID);

    if (!matchingPeer) {
        // if we don't have this sender we need to create them now
        matchingPeer = SharedNetworkPeer(new NetworkPeer(senderUUID, publicSocket, localSocket));
        _activePeers.insert(senderUUID, matchingPeer);

        qDebug() << "Added a new network peer" << *matchingPeer;
    } else {
        // we already had the peer so just potentially update their sockets
        matchingPeer->setPublicSocket(publicSocket);
        matchingPeer->setLocalSocket(localSocket);
    }

    // update our last heard microstamp for this network peer to now
    matchingPeer->setLastHeardMicrostamp(usecTimestampNow());

    return matchingPeer;
}

void IceServer::sendPeerInformationPacket(const NetworkPeer& peer, const HifiSockAddr* destinationSockAddr) {
    auto peerPacket { NLPacket::create(PacketType::IceServerPeerInformation); }

    // get the byte array for this peer
    peerPacket->write(peer.toByteArray());

    // write the current packet
    _serverSocket.writeDatagram(peerPacket.constData(), peerPacket.sizeUsed(),
                                destinationSockAddr->getAddress(), destinationSockAddr->getPort());
}

void IceServer::clearInactivePeers() {
    NetworkPeerHash::iterator peerItem = _activePeers.begin();

    while (peerItem != _activePeers.end()) {
        SharedNetworkPeer peer = peerItem.value();

        if ((usecTimestampNow() - peer->getLastHeardMicrostamp()) > (PEER_SILENCE_THRESHOLD_MSECS * 1000)) {
            qDebug() << "Removing peer from memory for inactivity -" << *peer;
            peerItem = _activePeers.erase(peerItem);
        } else {
            // we didn't kill this peer, push the iterator forwards
            ++peerItem;
        }
    }
}

bool IceServer::handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) {
    // We need an HTTP handler in order to monitor the health of the ice server
    // The correct functioning of the ICE server will be determined by its HTTP availability,

    if (connection->requestOperation() == QNetworkAccessManager::GetOperation) {
        if (url.path() == "/status") {
            connection->respond(HTTPConnection::StatusCode200, QByteArray::number(_activePeers.size()));
        }
    }
    return true;
}
