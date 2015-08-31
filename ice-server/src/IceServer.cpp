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
#include <udt/PacketHeaders.h>
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

    // set processPacket as the verified packet callback for the udt::Socket
    using std::placeholders::_1;
    _serverSocket.setPacketHandler(std::bind(&IceServer::processPacket, this, _1));
    
    // set packetVersionMatch as the verify packet operator for the udt::Socket
    _serverSocket.setPacketFilterOperator(std::bind(&IceServer::packetVersionMatch, this, _1));

    // setup our timer to clear inactive peers
    QTimer* inactivePeerTimer = new QTimer(this);
    connect(inactivePeerTimer, &QTimer::timeout, this, &IceServer::clearInactivePeers);
    inactivePeerTimer->start(CLEAR_INACTIVE_PEERS_INTERVAL_MSECS);

}

bool IceServer::packetVersionMatch(const udt::Packet& packet) {
    PacketType headerType = NLPacket::typeInHeader(packet);
    PacketVersion headerVersion = NLPacket::versionInHeader(packet);
    
    if (headerVersion == versionForPacketType(headerType)) {
        return true;
    } else {
        qDebug() << "Packet version mismatch for packet" << headerType
            << "(" << nameForPacketType(headerType) << ") from" << packet.getSenderSockAddr();
        
        return false;
    }
}

void IceServer::processPacket(std::unique_ptr<udt::Packet> packet) {
    
    auto nlPacket = NLPacket::fromBase(std::move(packet));
    
    // make sure that this packet at least looks like something we can read
    if (nlPacket->getPayloadSize() >= NLPacket::localHeaderSize(PacketType::ICEServerHeartbeat)) {
        
        if (nlPacket->getType() == PacketType::ICEServerHeartbeat) {
            SharedNetworkPeer peer = addOrUpdateHeartbeatingPeer(*nlPacket);
            
            // so that we can send packets to the heartbeating peer when we need, we need to activate a socket now
            peer->activateMatchingOrNewSymmetricSocket(nlPacket->getSenderSockAddr());
        } else if (nlPacket->getType() == PacketType::ICEServerQuery) {
            QDataStream heartbeatStream(nlPacket.get());
            
            // this is a node hoping to connect to a heartbeating peer - do we have the heartbeating peer?
            QUuid senderUUID;
            heartbeatStream >> senderUUID;
            
            // pull the public and private sock addrs for this peer
            HifiSockAddr publicSocket, localSocket;
            heartbeatStream >> publicSocket >> localSocket;
            
            // check if this node also included a UUID that they would like to connect to
            QUuid connectRequestID;
            heartbeatStream >> connectRequestID;
            
            SharedNetworkPeer matchingPeer = _activePeers.value(connectRequestID);
            
            if (matchingPeer) {
                
                qDebug() << "Sending information for peer" << connectRequestID << "to peer" << senderUUID;
                
                // we have the peer they want to connect to - send them pack the information for that peer
                sendPeerInformationPacket(*matchingPeer, &nlPacket->getSenderSockAddr());
                
                // we also need to send them to the active peer they are hoping to connect to
                // create a dummy peer object we can pass to sendPeerInformationPacket
                
                NetworkPeer dummyPeer(senderUUID, publicSocket, localSocket);
                sendPeerInformationPacket(dummyPeer, matchingPeer->getActiveSocket());
            } else {
                qDebug() << "Peer" << senderUUID << "asked for" << connectRequestID << "but no matching peer found";
            }
        }
    }
}

SharedNetworkPeer IceServer::addOrUpdateHeartbeatingPeer(NLPacket& packet) {

    // pull the UUID, public and private sock addrs for this peer
    QUuid senderUUID;
    HifiSockAddr publicSocket, localSocket;

    QDataStream heartbeatStream(&packet);
    
    heartbeatStream >> senderUUID;
    heartbeatStream >> publicSocket >> localSocket;

    // make sure we have this sender in our peer hash
    SharedNetworkPeer matchingPeer = _activePeers.value(senderUUID);

    if (!matchingPeer) {
        // if we don't have this sender we need to create them now
        matchingPeer = QSharedPointer<NetworkPeer>::create(senderUUID, publicSocket, localSocket);
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
    auto peerPacket = NLPacket::create(PacketType::ICEServerPeerInformation);

    // get the byte array for this peer
    peerPacket->write(peer.toByteArray());
    
    // write the current packet
    _serverSocket.writePacket(*peerPacket, *destinationSockAddr);
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
