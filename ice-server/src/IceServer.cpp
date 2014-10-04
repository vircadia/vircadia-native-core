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

IceServer::IceServer(int argc, char* argv[]) :
	QCoreApplication(argc, argv),
    _id(QUuid::createUuid()),
    _serverSocket(),
    _activePeers()
{
    // start the ice-server socket
    qDebug() << "ice-server socket is listening on" << ICE_SERVER_DEFAULT_PORT;
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
        
        
        if (packetTypeForPacket(incomingPacket) == PacketTypeIceServerHeartbeat) {
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
                
                qDebug() << "Matched hearbeat to existing network peer" << *matchingPeer;
            }
            
            // update our last heard microstamp for this network peer to now
            matchingPeer->setLastHeardMicrostamp(usecTimestampNow());
            
            // check if this node also included a UUID that they would like to connect to
            QUuid connectRequestID;
            hearbeatStream >> connectRequestID;
            
            // get the peers asking for connections with this peer
            QSet<QUuid>& requestingConnections = _currentConnections[senderUUID];
            
            if (!connectRequestID.isNull()) {
                qDebug() << "Peer wants to connect to peer with ID" << uuidStringWithoutCurlyBraces(connectRequestID);
                
                // ensure this peer is in the set of current connections for the peer with ID it wants to connect with
                _currentConnections[connectRequestID].insert(senderUUID);
                
                // add the ID of the node they have said they would like to connect to
                requestingConnections.insert(connectRequestID);
            }
            
            if (requestingConnections.size() > 0) {
                // send a heartbeart response based on the set of connections
                qDebug() << "Sending a heartbeat response to" << senderUUID << "who has" << requestingConnections.size()
                    << "potential connections";
                sendHeartbeatResponse(sendingSockAddr, requestingConnections);
            }
        }
    }
}

void IceServer::sendHeartbeatResponse(const HifiSockAddr& destinationSockAddr, QSet<QUuid>& connections) {
    QSet<QUuid>::iterator peerID = connections.begin();
    
    QByteArray outgoingPacket(MAX_PACKET_SIZE, 0);
    int currentPacketSize = populatePacketHeader(outgoingPacket, PacketTypeIceServerHeartbeatResponse, _id);
    int numHeaderBytes = currentPacketSize;
    
    // go through the connections, sending packets containing connection information for those nodes
    while (peerID != connections.end()) {
        SharedNetworkPeer matchingPeer = _activePeers.value(*peerID);
        // if this node is inactive we remove it from the set
        if (!matchingPeer) {
            peerID = connections.erase(peerID);
        } else {
            // get the byte array for this peer
            QByteArray peerBytes = matchingPeer->toByteArray();
            
            if (currentPacketSize + peerBytes.size() > MAX_PACKET_SIZE) {
                // write the current packet
                _serverSocket.writeDatagram(outgoingPacket.data(), currentPacketSize,
                                            destinationSockAddr.getAddress(), destinationSockAddr.getPort());
                
                // reset the packet size to our number of header bytes
                currentPacketSize = populatePacketHeader(outgoingPacket, PacketTypeIceServerHeartbeatResponse, _id);
            }
            
            // append the current peer bytes
            outgoingPacket.insert(currentPacketSize, peerBytes);
            currentPacketSize += peerBytes.size();
            
            ++peerID;
        }
    }
    
    if (currentPacketSize > numHeaderBytes) {
        // write the last packet, if there is data in it
        _serverSocket.writeDatagram(outgoingPacket.data(), currentPacketSize,
                                    destinationSockAddr.getAddress(), destinationSockAddr.getPort());
    }
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
