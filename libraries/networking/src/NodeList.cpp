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

#include <LogHandler.h>

#include "AccountManager.h"
#include "AddressManager.h"
#include "Assignment.h"
#include "HifiSockAddr.h"
#include "JSONBreakableMarshal.h"
#include "NodeList.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "UUID.h"
#include "NetworkLogging.h"

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
    static bool firstCall = true;
    if (firstCall) {
        NodeType::init();
        // register the SharedNodePointer meta-type for signals/slots
        qRegisterMetaType<SharedNodePointer>();
        firstCall = false;
    }
    auto addressManager = DependencyManager::get<AddressManager>();
    
    // handle domain change signals from AddressManager
    connect(addressManager.data(), &AddressManager::possibleDomainChangeRequired,
            &_domainHandler, &DomainHandler::setHostnameAndPort);
    
    connect(addressManager.data(), &AddressManager::possibleDomainChangeRequiredViaICEForID,
            &_domainHandler, &DomainHandler::setIceServerHostnameAndID);
    
    // clear our NodeList when the domain changes
    connect(&_domainHandler, &DomainHandler::disconnectedFromDomain, this, &NodeList::reset);
    
    // handle ICE signal from DS so connection is attempted immediately
    connect(&_domainHandler, &DomainHandler::requestICEConnectionAttempt, this, &NodeList::handleICEConnectionToDomainServer);

    // clear out NodeList when login is finished
    connect(&AccountManager::getInstance(), &AccountManager::loginComplete , this, &NodeList::reset);
    
    // clear our NodeList when logout is requested
    connect(&AccountManager::getInstance(), &AccountManager::logoutComplete , this, &NodeList::reset);
}

qint64 NodeList::sendStats(const QJsonObject& statsObject, const HifiSockAddr& destination) {
    QByteArray statsPacket(MAX_PACKET_SIZE, 0);
    int numBytesForPacketHeader = populatePacketHeader(statsPacket, PacketTypeNodeJsonStats);

    // get a QStringList using JSONBreakableMarshal
    QStringList statsStringList = JSONBreakableMarshal::toStringList(statsObject, "");

    int numBytesWritten = numBytesForPacketHeader;
    
    // enumerate the resulting strings - pack them and send off packets once we hit MTU size 
    foreach(const QString& statsItem, statsStringList) {
        QByteArray utf8String = statsItem.toUtf8();
        utf8String.append('\0');

        if (numBytesWritten + utf8String.size() > MAX_PACKET_SIZE) {
            // send off the current packet since the next string will make us too big
            statsPacket.resize(numBytesWritten);
            writeUnverifiedDatagram(statsPacket, destination);
           
            // reset the number of bytes written to the size of our packet header
            numBytesWritten = numBytesForPacketHeader;
        }
        
        // write this string into the stats packet
        statsPacket.replace(numBytesWritten, utf8String.size(), utf8String);
        
        // keep track of the number of bytes we have written
        numBytesWritten += utf8String.size();
    }

    if (numBytesWritten > numBytesForPacketHeader) {
        // always send the last packet, if it has data
        statsPacket.resize(numBytesWritten);
        writeUnverifiedDatagram(statsPacket, destination);
    }
    
    // enumerate the resulting strings, breaking them into MTU sized packets
    return 0;
}

qint64 NodeList::sendStatsToDomainServer(const QJsonObject& statsObject) {
    return sendStats(statsObject, _domainHandler.getSockAddr());
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
        qCDebug(networking) << "PING_REPLY from node " << *sendingNode << "\n" <<
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
            qCDebug(networking) << "Received reply from domain-server on" << senderSockAddr;
            
            // for now we're unsafely assuming this came back from the domain
            if (senderSockAddr == _domainHandler.getICEPeer().getLocalSocket()) {
                qCDebug(networking) << "Connecting to domain using local socket";
                _domainHandler.activateICELocalSocket();
            } else if (senderSockAddr == _domainHandler.getICEPeer().getPublicSocket()) {
                qCDebug(networking) << "Conecting to domain using public socket";
                _domainHandler.activateICEPublicSocket();
            } else {
                qCDebug(networking) << "Reply does not match either local or public socket for domain. Will not connect.";
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
    
    if (sender() != &_domainHandler) {
        // clear the domain connection information, unless they're the ones that asked us to reset
        _domainHandler.softReset();
    }
    
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
        qCDebug(networking) << "Sending intial stun request to" << STUN_SERVER_HOSTNAME;
    }
    
    LimitedNodeList::sendSTUNRequest();

    _stunRequestsSinceSuccess++;

    if (_stunRequestsSinceSuccess >= NUM_STUN_REQUESTS_BEFORE_FALLBACK) {
        if (!_hasCompletedInitialSTUNFailure) {
            // if we're here this was the last failed STUN request
            // use our DS as our stun server
            qCDebug(networking, "Failed to lookup public address via STUN server at %s:%hu. Using DS for STUN.",
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
            qCDebug(networking) << "Sending connect request to domain-server at" << _domainHandler.getHostname();
            
            // is this our localhost domain-server?
            // if so we need to make sure we have an up-to-date local port in case it restarted
            
            if (_domainHandler.getSockAddr().getAddress() == QHostAddress::LocalHost
                || _domainHandler.getHostname() == "localhost") {
                
                static QSharedMemory* localDSPortSharedMem = NULL;
                
                quint16 domainPort = DEFAULT_DOMAIN_SERVER_PORT;
                getLocalServerPortFromSharedMemory(DOMAIN_SERVER_LOCAL_PORT_SMEM_KEY,
                                                   localDSPortSharedMem,
                                                   domainPort);
                qCDebug(networking) << "Local domain-server port read from shared memory (or default) is" << domainPort;
                _domainHandler.setPort(domainPort);
            }
            
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
        packetStream << _ownerType << _publicSockAddr << _localSockAddr << _nodeTypesOfInterest.toList();
        
        
        // if this is a connect request, and we can present a username signature, send it along
        if (!_domainHandler.isConnected()) {
            DataServerAccountInfo& accountInfo = AccountManager::getInstance().getAccountInfo();
            packetStream << accountInfo.getUsername();
            
            const QByteArray& usernameSignature = AccountManager::getInstance().getAccountInfo().getUsernameSignature();
            
            if (!usernameSignature.isEmpty()) {
                qCDebug(networking) << "Including username signature in domain connect request.";
                packetStream << usernameSignature;
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
            // so emit our signal that says that
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
        qCDebug(networking) << "Sending ping packets to establish connectivity with domain-server with ID"
            << uuidStringWithoutCurlyBraces(_domainHandler.getICEDomainID());
        
        // send the ping packet to the local and public sockets for this node
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
    
    
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    // pull our owner UUID from the packet, it's always the first thing
    QUuid newUUID;
    packetStream >> newUUID;
    setSessionUUID(newUUID);

    bool thisNodeCanAdjustLocks;
    packetStream >> thisNodeCanAdjustLocks;
    setThisNodeCanAdjustLocks(thisNodeCanAdjustLocks);

    bool thisNodeCanRez;
    packetStream >> thisNodeCanRez;
    setThisNodeCanRez(thisNodeCanRez);
    
    // pull each node in the packet
    while(packetStream.device()->pos() < packet.size()) {
        // setup variables to read into from QDataStream
        qint8 nodeType;
        QUuid nodeUUID, connectionUUID;
        HifiSockAddr nodePublicSocket, nodeLocalSocket;
        bool canAdjustLocks;
        bool canRez;

        packetStream >> nodeType >> nodeUUID >> nodePublicSocket >> nodeLocalSocket >> canAdjustLocks >> canRez;

        // if the public socket address is 0 then it's reachable at the same IP
        // as the domain server
        if (nodePublicSocket.getAddress().isNull()) {
            nodePublicSocket.setAddress(_domainHandler.getIP());
        }

        SharedNodePointer node = addOrUpdateNode(nodeUUID, nodeType, nodePublicSocket,
                                                 nodeLocalSocket, canAdjustLocks, canRez);
        
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
    eachNode([this](const SharedNodePointer& node){
        if (!node->getActiveSocket()) {
            // we don't have an active link to this node, ping it to set that up
            pingPunchForInactiveNode(node);
        }
    });
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
