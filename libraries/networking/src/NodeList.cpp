//
//  NodeList.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QUrl>
#include <QtNetwork/QHostInfo>

#include "AccountManager.h"
#include "Assignment.h"
#include "HifiSockAddr.h"
#include "Logging.h"
#include "NodeList.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "UUID.h"

NodeList* NodeList::createInstance(char ownerType, unsigned short socketListenPort, unsigned short dtlsPort) {
    
    NodeType::init();
    
    if (_sharedInstance.get()) {
        qDebug() << "NodeList called with existing instance." <<
        "Releasing auto_ptr, deleting existing instance and creating a new one.";
        
        delete _sharedInstance.release();
    }
    
    _sharedInstance = std::auto_ptr<LimitedNodeList>(new NodeList(ownerType, socketListenPort, dtlsPort));
    
    // register the SharedNodePointer meta-type for signals/slots
    qRegisterMetaType<SharedNodePointer>();
    
    return static_cast<NodeList*>(_sharedInstance.get());
}

NodeList* NodeList::getInstance() {
    if (!_sharedInstance.get()) {
        qDebug("NodeList getInstance called before call to createInstance. Returning NULL pointer.");
    }

    return static_cast<NodeList*>(_sharedInstance.get());
}

NodeList::NodeList(char newOwnerType, unsigned short socketListenPort, unsigned short dtlsListenPort) :
    LimitedNodeList(socketListenPort, dtlsListenPort),
    _ownerType(newOwnerType),
    _nodeTypesOfInterest(),
    _domainHandler(this),
    _numNoReplyDomainCheckIns(0),
    _assignmentServerSocket(),
    _hasCompletedInitialSTUNFailure(false),
    _stunRequestsSinceSuccess(0)
{
    // clear our NodeList when the domain changes
    connect(&_domainHandler, &DomainHandler::hostnameChanged, this, &NodeList::reset);
    
    // handle ICE signal from DS so connection is attempted immediately
    connect(&_domainHandler, &DomainHandler::requestICEConnectionAttempt, this, &NodeList::handleICEConnectionToDomainServer);
    
    // clear our NodeList when logout is requested
    connect(&AccountManager::getInstance(), &AccountManager::logoutComplete , this, &NodeList::reset);
}

qint64 NodeList::sendStatsToDomainServer(const QJsonObject& statsObject) {
    QByteArray statsPacket = byteArrayWithPopulatedHeader(PacketTypeNodeJsonStats);
    QDataStream statsPacketStream(&statsPacket, QIODevice::Append);
    
    statsPacketStream << statsObject.toVariantMap();
    
    return writeUnverifiedDatagram(statsPacket, _domainHandler.getSockAddr());
}

void NodeList::timePingReply(const QByteArray& packet, const SharedNodePointer& sendingNode) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    quint8 pingType;
    quint64 ourOriginalTime, othersReplyTime;
    
    packetStream >> pingType >> ourOriginalTime >> othersReplyTime;
    
    quint64 now = usecTimestampNow();
    int pingTime = now - ourOriginalTime;
    int oneWayFlightTime = pingTime / 2; // half of the ping is our one way flight
    
    // The other node's expected time should be our original time plus the one way flight time
    // anything other than that is clock skew
    quint64 othersExprectedReply = ourOriginalTime + oneWayFlightTime;
    int clockSkew = othersReplyTime - othersExprectedReply;
    
    sendingNode->setPingMs(pingTime / 1000);
    sendingNode->updateClockSkewUsec(clockSkew);

    const bool wantDebug = false;
    
    if (wantDebug) {
        qDebug() << "PING_REPLY from node " << *sendingNode << "\n" <<
        "                     now: " << now << "\n" <<
        "                 ourTime: " << ourOriginalTime << "\n" <<
        "                pingTime: " << pingTime << "\n" <<
        "        oneWayFlightTime: " << oneWayFlightTime << "\n" <<
        "         othersReplyTime: " << othersReplyTime << "\n" <<
        "    othersExprectedReply: " << othersExprectedReply << "\n" <<
        "               clockSkew: " << clockSkew  << "\n" <<
        "       average clockSkew: " << sendingNode->getClockSkewUsec();
    }
}

void NodeList::processNodeData(const HifiSockAddr& senderSockAddr, const QByteArray& packet) {
    switch (packetTypeForPacket(packet)) {
        case PacketTypeDomainList: {
            processDomainServerList(packet);
            break;
        }
        case PacketTypeDomainServerRequireDTLS: {
            _domainHandler.parseDTLSRequirementPacket(packet);
            break;
        }
        case PacketTypeIceServerHeartbeatResponse: {
            _domainHandler.processICEResponsePacket(packet);
            break;
        }
        case PacketTypePing: {
            // send back a reply
            SharedNodePointer matchingNode = sendingNodeForPacket(packet);
            if (matchingNode) {
                matchingNode->setLastHeardMicrostamp(usecTimestampNow());
                QByteArray replyPacket = constructPingReplyPacket(packet);
                writeDatagram(replyPacket, matchingNode, senderSockAddr);
                
                // If we don't have a symmetric socket for this node and this socket doesn't match
                // what we have for public and local then set it as the symmetric.
                // This allows a server on a reachable port to communicate with nodes on symmetric NATs
                if (matchingNode->getSymmetricSocket().isNull()) {
                    if (senderSockAddr != matchingNode->getLocalSocket() && senderSockAddr != matchingNode->getPublicSocket()) {
                        matchingNode->setSymmetricSocket(senderSockAddr);
                    }
                }
            }
            
            break;
        }
        case PacketTypePingReply: {
            SharedNodePointer sendingNode = sendingNodeForPacket(packet);
            
            if (sendingNode) {
                sendingNode->setLastHeardMicrostamp(usecTimestampNow());
                
                // activate the appropriate socket for this node, if not yet updated
                activateSocketFromNodeCommunication(packet, sendingNode);
                
                // set the ping time for this node for stat collection
                timePingReply(packet, sendingNode);
            }
            
            break;
        }
        case PacketTypeUnverifiedPing: {
            // send back a reply
            QByteArray replyPacket = constructPingReplyPacket(packet, _domainHandler.getICEClientID());
            writeUnverifiedDatagram(replyPacket, senderSockAddr);
            break;
        }
        case PacketTypeUnverifiedPingReply: {
            qDebug() << "Received reply from domain-server on" << senderSockAddr;
            
            // for now we're unsafely assuming this came back from the domain
            if (senderSockAddr == _domainHandler.getICEPeer().getLocalSocket()) {
                qDebug() << "Connecting to domain using local socket";
                _domainHandler.activateICELocalSocket();
            } else if (senderSockAddr == _domainHandler.getICEPeer().getPublicSocket()) {
                qDebug() << "Conecting to domain using public socket";
                _domainHandler.activateICEPublicSocket();
            } else {
                qDebug() << "Reply does not match either local or public socket for domain. Will not connect.";
            }
        }
        case PacketTypeStunResponse: {
            // a STUN packet begins with 00, we've checked the second zero with packetVersionMatch
            // pass it along so it can be processed into our public address and port
            processSTUNResponse(packet);
            break;
        }
        default:
            LimitedNodeList::processNodeData(senderSockAddr, packet);
            break;
    }
}

void NodeList::reset() {
    LimitedNodeList::reset();
    
    _numNoReplyDomainCheckIns = 0;

    // refresh the owner UUID to the NULL UUID
    setSessionUUID(QUuid());
    
    // clear the domain connection information
    _domainHandler.softReset();
    
    // if we setup the DTLS socket, also disconnect from the DTLS socket readyRead() so it can handle handshaking
    if (_dtlsSocket) {
        disconnect(_dtlsSocket, 0, this, 0);
    }
}

void NodeList::addNodeTypeToInterestSet(NodeType_t nodeTypeToAdd) {
    _nodeTypesOfInterest << nodeTypeToAdd;
}

void NodeList::addSetOfNodeTypesToNodeInterestSet(const NodeSet& setOfNodeTypes) {
    _nodeTypesOfInterest.unite(setOfNodeTypes);
}


const unsigned int NUM_STUN_REQUESTS_BEFORE_FALLBACK = 5;

void NodeList::sendSTUNRequest() {

    if (!_hasCompletedInitialSTUNFailure) {
        qDebug() << "Sending intial stun request to" << STUN_SERVER_HOSTNAME;
    }
    
    LimitedNodeList::sendSTUNRequest();

    _stunRequestsSinceSuccess++;

    if (_stunRequestsSinceSuccess >= NUM_STUN_REQUESTS_BEFORE_FALLBACK) {
        if (!_hasCompletedInitialSTUNFailure) {
            // if we're here this was the last failed STUN request
            // use our DS as our stun server
            qDebug("Failed to lookup public address via STUN server at %s:%hu. Using DS for STUN.",
                   STUN_SERVER_HOSTNAME, STUN_SERVER_PORT);

            _hasCompletedInitialSTUNFailure = true;
        }

        // reset the public address and port
        // use 0 so the DS knows to act as out STUN server
        _publicSockAddr = HifiSockAddr(QHostAddress(), _nodeSocket.localPort());
    }
}

bool NodeList::processSTUNResponse(const QByteArray& packet) {
    if (LimitedNodeList::processSTUNResponse(packet)) {
        // reset the number of failed STUN requests since last success
        _stunRequestsSinceSuccess = 0;
        
        _hasCompletedInitialSTUNFailure = true;
        
        return true;
    } else {
        return false;
    }
}

void NodeList::sendDomainServerCheckIn() {
    if (_publicSockAddr.isNull() && !_hasCompletedInitialSTUNFailure) {
        // we don't know our public socket and we need to send it to the domain server
        // send a STUN request to figure it out
        sendSTUNRequest();
    } else if (_domainHandler.getIP().isNull() && _domainHandler.requiresICE()) {
        handleICEConnectionToDomainServer();
    } else if (!_domainHandler.getIP().isNull()) {
        
        bool isUsingDTLS = false;
        
        PacketType domainPacketType = !_domainHandler.isConnected()
            ? PacketTypeDomainConnectRequest : PacketTypeDomainListRequest;
        
        if (!_domainHandler.isConnected()) {
            qDebug() << "Sending connect request to domain-server at" << _domainHandler.getHostname();
        }
        
        // construct the DS check in packet
        QUuid packetUUID = _sessionUUID;
        
        if (domainPacketType == PacketTypeDomainConnectRequest) {
            if (!_domainHandler.getAssignmentUUID().isNull()) {
                // this is a connect request and we're an assigned node
                // so set our packetUUID as the assignment UUID
                packetUUID = _domainHandler.getAssignmentUUID();
            } else if (_domainHandler.requiresICE()) {
                // this is a connect request and we're an interface client
                // that used ice to discover the DS
                // so send our ICE client UUID with the connect request
                packetUUID = _domainHandler.getICEClientID();
            }
        }
        
        QByteArray domainServerPacket = byteArrayWithPopulatedHeader(domainPacketType, packetUUID);
        QDataStream packetStream(&domainServerPacket, QIODevice::Append);
        
        // pack our data to send to the domain-server
        packetStream << _ownerType << _publicSockAddr << _localSockAddr << (quint8) _nodeTypesOfInterest.size();
        
        // copy over the bytes for node types of interest, if required
        foreach (NodeType_t nodeTypeOfInterest, _nodeTypesOfInterest) {
            packetStream << nodeTypeOfInterest;
        }
        
        // if this is a connect request, and we can present a username signature, send it along
        if (!_domainHandler.isConnected()) {
            const QByteArray& usernameSignature = AccountManager::getInstance().getAccountInfo().usernameSignature();
            
            if (!usernameSignature.isEmpty()) {
                qDebug() << "Including username signature in domain connect request.";
                packetStream << usernameSignature;
            } else {
                qDebug() << "Private key not present - domain connect request cannot include username signature";
            }
        }
        
        if (!isUsingDTLS) {
            writeDatagram(domainServerPacket, _domainHandler.getSockAddr(), QUuid());
        }
        
        const int NUM_DOMAIN_SERVER_CHECKINS_PER_STUN_REQUEST = 5;
        static unsigned int numDomainCheckins = 0;
        
        // send a STUN request every Nth domain server check in so we update our public socket, if required
        if (numDomainCheckins++ % NUM_DOMAIN_SERVER_CHECKINS_PER_STUN_REQUEST == 0) {
            sendSTUNRequest();
        }
        
        if (_numNoReplyDomainCheckIns >= MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            // we haven't heard back from DS in MAX_SILENT_DOMAIN_SERVER_CHECK_INS
            // so emit our signal that indicates that
            emit limitOfSilentDomainCheckInsReached();
        }
        
        // increment the count of un-replied check-ins
        _numNoReplyDomainCheckIns++;
    }
}

void NodeList::handleICEConnectionToDomainServer() {
    if (_domainHandler.getICEPeer().isNull()
        || _domainHandler.getICEPeer().getConnectionAttempts() >= MAX_ICE_CONNECTION_ATTEMPTS) {
        
        _domainHandler.getICEPeer().resetConnectionAttemps();
        
        LimitedNodeList::sendHeartbeatToIceServer(_domainHandler.getICEServerSockAddr(),
                                                  _domainHandler.getICEClientID(),
                                                  _domainHandler.getICEDomainID());
    } else {
        qDebug() << "Sending ping packets to establish connectivity with domain-server with ID"
            << uuidStringWithoutCurlyBraces(_domainHandler.getICEDomainID());
        
        // send the ping packet to the local and public sockets for this nodfe
        QByteArray localPingPacket = constructPingPacket(PingType::Local, false, _domainHandler.getICEClientID());
        writeUnverifiedDatagram(localPingPacket, _domainHandler.getICEPeer().getLocalSocket());
        
        QByteArray publicPingPacket = constructPingPacket(PingType::Public, false, _domainHandler.getICEClientID());
        writeUnverifiedDatagram(publicPingPacket, _domainHandler.getICEPeer().getPublicSocket());
        
        _domainHandler.getICEPeer().incrementConnectionAttempts();
    }
}

int NodeList::processDomainServerList(const QByteArray& packet) {
    // this is a packet from the domain server, reset the count of un-replied check-ins
    _numNoReplyDomainCheckIns = 0;
    
    // if this was the first domain-server list from this domain, we've now connected
    if (!_domainHandler.isConnected()) {
        _domainHandler.setUUID(uuidFromPacketHeader(packet));
        _domainHandler.setIsConnected(true);
    }

    int readNodes = 0;
    
    // setup variables to read into from QDataStream
    qint8 nodeType;
    
    QUuid nodeUUID, connectionUUID;

    HifiSockAddr nodePublicSocket;
    HifiSockAddr nodeLocalSocket;
    
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    // pull our owner UUID from the packet, it's always the first thing
    QUuid newUUID;
    packetStream >> newUUID;
    setSessionUUID(newUUID);
    
    // pull each node in the packet
    while(packetStream.device()->pos() < packet.size()) {
        packetStream >> nodeType >> nodeUUID >> nodePublicSocket >> nodeLocalSocket;

        // if the public socket address is 0 then it's reachable at the same IP
        // as the domain server
        if (nodePublicSocket.getAddress().isNull()) {
            nodePublicSocket.setAddress(_domainHandler.getIP());
        }

        SharedNodePointer node = addOrUpdateNode(nodeUUID, nodeType, nodePublicSocket, nodeLocalSocket);
        
        packetStream >> connectionUUID;
        node->setConnectionSecret(connectionUUID);
    }
    
    // ping inactive nodes in conjunction with receipt of list from domain-server
    // this makes it happen every second and also pings any newly added nodes
    pingInactiveNodes();

    return readNodes;
}

void NodeList::sendAssignment(Assignment& assignment) {
    
    PacketType assignmentPacketType = assignment.getCommand() == Assignment::CreateCommand
        ? PacketTypeCreateAssignment
        : PacketTypeRequestAssignment;
    
    QByteArray packet = byteArrayWithPopulatedHeader(assignmentPacketType);
    QDataStream packetStream(&packet, QIODevice::Append);
    
    packetStream << assignment;

    _nodeSocket.writeDatagram(packet, _assignmentServerSocket.getAddress(), _assignmentServerSocket.getPort());
}

void NodeList::pingPunchForInactiveNode(const SharedNodePointer& node) {
    
    // send the ping packet to the local and public sockets for this node
    QByteArray localPingPacket = constructPingPacket(PingType::Local);
    writeDatagram(localPingPacket, node, node->getLocalSocket());
    
    QByteArray publicPingPacket = constructPingPacket(PingType::Public);
    writeDatagram(publicPingPacket, node, node->getPublicSocket());
    
    if (!node->getSymmetricSocket().isNull()) {
        QByteArray symmetricPingPacket = constructPingPacket(PingType::Symmetric);
        writeDatagram(symmetricPingPacket, node, node->getSymmetricSocket());
    }
}

void NodeList::pingInactiveNodes() {
    foreach (const SharedNodePointer& node, getNodeHash()) {
        if (!node->getActiveSocket()) {
            // we don't have an active link to this node, ping it to set that up
            pingPunchForInactiveNode(node);
        }
    }
}

void NodeList::activateSocketFromNodeCommunication(const QByteArray& packet, const SharedNodePointer& sendingNode) {
    // deconstruct this ping packet to see if it is a public or local reply
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    quint8 pingType;
    packetStream >> pingType;
    
    // if this is a local or public ping then we can activate a socket
    // we do nothing with agnostic pings, those are simply for timing
    if (pingType == PingType::Local && sendingNode->getActiveSocket() != &sendingNode->getLocalSocket()) {
        sendingNode->activateLocalSocket();
    } else if (pingType == PingType::Public && !sendingNode->getActiveSocket()) {
        sendingNode->activatePublicSocket();
    } else if (pingType == PingType::Symmetric && !sendingNode->getActiveSocket()) {
        sendingNode->activateSymmetricSocket();
    }
}
