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
#include <QtCore/QMetaEnum>
#include <QtCore/QUrl>
#include <QtCore/QThread>
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
    _assignmentServerSocket()
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

    // handle a request for a path change from the AddressManager
    connect(addressManager.data(), &AddressManager::pathChangeRequired, this, &NodeList::handleDSPathQuery);

    // in case we don't know how to talk to DS when a path change is requested
    // fire off any pending DS path query when we get socket information
    connect(&_domainHandler, &DomainHandler::completedSocketDiscovery, this, &NodeList::sendPendingDSPathQuery);

    // send a domain server check in immediately once the DS socket is known
    connect(&_domainHandler, &DomainHandler::completedSocketDiscovery, this, &NodeList::sendDomainServerCheckIn);

    // send a domain server check in immediately if there is a public socket change
    connect(this, &LimitedNodeList::publicSockAddrChanged, this, &NodeList::sendDomainServerCheckIn);

    // clear our NodeList when the domain changes
    connect(&_domainHandler, &DomainHandler::disconnectedFromDomain, this, &NodeList::reset);

    // send an ICE heartbeat as soon as we get ice server information
    connect(&_domainHandler, &DomainHandler::iceSocketAndIDReceived, this, &NodeList::handleICEConnectionToDomainServer);

    // handle ping timeout from DomainHandler to establish a connection with auto networked domain-server
    connect(&_domainHandler.getICEPeer(), &NetworkPeer::pingTimerTimeout, this, &NodeList::pingPunchForDomainServer);

    // send a ping punch immediately
    connect(&_domainHandler, &DomainHandler::icePeerSocketsReceived, this, &NodeList::pingPunchForDomainServer);

    // clear out NodeList when login is finished
    connect(&AccountManager::getInstance(), &AccountManager::loginComplete , this, &NodeList::reset);

    // clear our NodeList when logout is requested
    connect(&AccountManager::getInstance(), &AccountManager::logoutComplete , this, &NodeList::reset);

    // anytime we get a new node we will want to attempt to punch to it
    connect(this, &LimitedNodeList::nodeAdded, this, &NodeList::startNodeHolePunch);

    // we definitely want STUN to update our public socket, so call the LNL to kick that off
    startSTUNPublicSocketUpdate();
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
    PacketType::Value packetType = packetTypeForPacket(packet);
    switch (packetType) {
        case PacketType::DomainList:
        case PacketType::DomainServerAddedNode: {
            if (!_domainHandler.getSockAddr().isNull()) {
                // only process a packet from domain-server if we're talking to a domain
                // TODO: how do we make sure this is actually the domain we want the list from (DTLS probably)
                if (packetType == PacketType::DomainList) {
                    processDomainServerList(packet);
                } else if (packetType == PacketType::DomainServerAddedNode) {
                    processDomainServerAddedNode(packet);
                }
            }
            break;
        }
        case PacketType::DomainServerRequireDTLS: {
            _domainHandler.parseDTLSRequirementPacket(packet);
            break;
        }
        case PacketType::IceServerPeerInformation: {
            if (!_domainHandler.getICEPeer().hasSockets()) {
                _domainHandler.processICEResponsePacket(packet);
            }
            break;
        }
        case PacketType::Ping: {
            // send back a reply
            SharedNodePointer matchingNode = sendingNodeForPacket(packet);
            if (matchingNode) {
                matchingNode->setLastHeardMicrostamp(usecTimestampNow());
                auto replyPacket = constructPingReplyPacket(packet);
                sendPacket(replyPacket, matchingNode, senderSockAddr);

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
        case PacketType::PingReply: {
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
        case PacketType::ICEPing: {
            // send back a reply
            auto replyPacket = constructICEPingReplyPacket(packet, _domainHandler.getICEClientID());
            sendPacket(replyPacket, senderSockAddr);
            break;
        }
        case PacketType::ICEPingReply: {
            qCDebug(networking) << "Received reply from domain-server on" << senderSockAddr;

            if (_domainHandler.getIP().isNull()) {
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
        }
        case PacketType::StunResponse: {
            // a STUN packet begins with 00, we've checked the second zero with packetVersionMatch
            // pass it along so it can be processed into our public address and port
            processSTUNResponse(packet);
            break;
        }
        case PacketType::DomainServerPathResponse: {
            handleDSPathQueryResponse(packet);
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

void NodeList::sendDomainServerCheckIn() {
    if (_publicSockAddr.isNull()) {
        // we don't know our public socket and we need to send it to the domain server
        qCDebug(networking) << "Waiting for inital public socket from STUN. Will not send domain-server check in.";
    } else if (_domainHandler.getIP().isNull() && _domainHandler.requiresICE()) {
        qCDebug(networking) << "Waiting for ICE discovered domain-server socket. Will not send domain-server check in.";
        handleICEConnectionToDomainServer();
    } else if (!_domainHandler.getIP().isNull()) {
        bool isUsingDTLS = false;

        PacketType::Value domainPacketType = !_domainHandler.isConnected()
            ? PacketType::DomainConnectRequest : PacketType::DomainListRequest;

        if (!_domainHandler.isConnected()) {
            qCDebug(networking) << "Sending connect request to domain-server at" << _domainHandler.getHostname();

            // is this our localhost domain-server?
            // if so we need to make sure we have an up-to-date local port in case it restarted

            if (_domainHandler.getSockAddr().getAddress() == QHostAddress::LocalHost
                || _domainHandler.getHostname() == "localhost") {

                quint16 domainPort = DEFAULT_DOMAIN_SERVER_PORT;
                getLocalServerPortFromSharedMemory(DOMAIN_SERVER_LOCAL_PORT_SMEM_KEY, domainPort);
                qCDebug(networking) << "Local domain-server port read from shared memory (or default) is" << domainPort;
                _domainHandler.setPort(domainPort);
            }

        }

        auto domainPacket { NLPacket::create(domainPacketType); }
        QDataStream packetStream(&domainPacket->getPayload);

        if (domainPacketType == PacketType::DomainConnectRequest) {
            QUuid connectUUID;

            if (!_domainHandler.getAssignmentUUID().isNull()) {
                // this is a connect request and we're an assigned node
                // so set our packetUUID as the assignment UUID
                connectUUID = _domainHandler.getAssignmentUUID();
            } else if (_domainHandler.requiresICE()) {
                // this is a connect request and we're an interface client
                // that used ice to discover the DS
                // so send our ICE client UUID with the connect request
                connectUUID = _domainHandler.getICEClientID();
            }

            // pack the connect UUID for this connect request
            packetStream << connectUUID;
        }

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

        flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendDSCheckIn);

        if (!isUsingDTLS) {
            sendPacket(domainPacket, _domainHandler.getSockAddr());
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

void NodeList::handleDSPathQuery(const QString& newPath) {
    if (_domainHandler.isSocketKnown()) {
        // if we have a DS socket we assume it will get this packet and send if off right away
        sendDSPathQuery(newPath);
    } else {
        // otherwise we make it pending so that it can be sent once a connection is established
        _domainHandler.setPendingPath(newPath);
    }
}

void NodeList::sendPendingDSPathQuery() {

    QString pendingPath = _domainHandler.getPendingPath();

    if (!pendingPath.isEmpty()) {
        qCDebug(networking) << "Attemping to send pending query to DS for path" << pendingPath;

        // this is a slot triggered if we just established a network link with a DS and want to send a path query
        sendDSPathQuery(_domainHandler.getPendingPath());

        // clear whatever the pending path was
        _domainHandler.clearPendingPath();
    }
}

void NodeList::sendDSPathQuery(const QString& newPath) {
    // only send a path query if we know who our DS is or is going to be
    if (_domainHandler.isSocketKnown()) {
        // construct the path query packet
        auto pathQueryPacket = NLPacket::create(PacketType::DomainServerPathQuery);

        // get the UTF8 representation of path query
        QByteArray pathQueryUTF8 = newPath.toUtf8();

        // get the size of the UTF8 representation of the desired path
        quint16 numPathBytes = pathQueryUTF8.size();

        if (numPathBytes + sizeof(numPathBytes) < pathQueryPacket.size() ) {
            // append the size of the path to the query packet
            pathQueryPacket.write(&numPathBytes, sizeof(numPathBytes));

            // append the path itself to the query packet
            pathQueryPacket.write(pathQueryUTF8);

            qCDebug(networking) << "Sending a path query packet for path" << newPath << "to domain-server at"
                << _domainHandler.getSockAddr();

            // send off the path query
            sendPacket(pathQueryPacket, _domainHandler.getSockAddr());
        } else {
            qCDebug(networking) << "Path" << newPath << "would make PacketTypeDomainServerPathQuery packet > MAX_PACKET_SIZE." <<
                "Will not send query.";
        }
    }
}

void NodeList::handleDSPathQueryResponse(const QByteArray& packet) {
    // This is a response to a path query we theoretically made.
    // In the future we may want to check that this was actually from our DS and for a query we actually made.

    int numHeaderBytes = numBytesForPacketHeaderGivenPacketType(PacketTypeDomainServerPathResponse);
    const char* startPosition = packet.data() + numHeaderBytes;
    const char* currentPosition = startPosition;

    // figure out how many bytes the path query is
    qint16 numPathBytes;
    memcpy(&numPathBytes, currentPosition, sizeof(numPathBytes));
    currentPosition += sizeof(numPathBytes);

    // make sure it is safe to pull the path
    if (numPathBytes <= packet.size() - numHeaderBytes - (currentPosition - startPosition)) {
        // pull the path from the packet
        QString pathQuery = QString::fromUtf8(currentPosition, numPathBytes);
        currentPosition += numPathBytes;

        // figure out how many bytes the viewpoint is
        qint16 numViewpointBytes;
        memcpy(&numViewpointBytes, currentPosition, sizeof(numViewpointBytes));
        currentPosition += sizeof(numViewpointBytes);

        // make sure it is safe to pull the viewpoint
        if (numViewpointBytes <= packet.size() - numHeaderBytes - (currentPosition - startPosition)) {
            // pull the viewpoint from the packet
            QString viewpoint = QString::fromUtf8(currentPosition, numViewpointBytes);

            // Hand it off to the AddressManager so it can handle it as a relative viewpoint
            if (DependencyManager::get<AddressManager>()->goToViewpointForPath(viewpoint, pathQuery)) {
                qCDebug(networking) << "Going to viewpoint" << viewpoint << "which was the lookup result for path" << pathQuery;
            } else {
                qCDebug(networking) << "Could not go to viewpoint" << viewpoint
                    << "which was the lookup result for path" << pathQuery;
            }
        }
    }
}

void NodeList::handleICEConnectionToDomainServer() {
    // if we're still waiting to get sockets we want to ping for the domain-server
    // then send another heartbeat now
    if (!_domainHandler.getICEPeer().hasSockets()) {

        _domainHandler.getICEPeer().resetConnectionAttempts();

        flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendICEServerQuery);

        LimitedNodeList::sendPeerQueryToIceServer(_domainHandler.getICEServerSockAddr(),
                                                  _domainHandler.getICEClientID(),
                                                  _domainHandler.getICEDomainID());
    }
}

void NodeList::pingPunchForDomainServer() {
    // make sure if we're here that we actually still need to ping the domain-server
    if (_domainHandler.getIP().isNull() && _domainHandler.getICEPeer().hasSockets()) {

        // check if we've hit the number of pings we'll send to the DS before we consider it a fail
        const int NUM_DOMAIN_SERVER_PINGS_BEFORE_RESET = 2000 / UDP_PUNCH_PING_INTERVAL_MS;

        if (_domainHandler.getICEPeer().getConnectionAttempts() == 0) {
            qCDebug(networking) << "Sending ping packets to establish connectivity with domain-server with ID"
                << uuidStringWithoutCurlyBraces(_domainHandler.getICEDomainID());
        } else {
            if (_domainHandler.getICEPeer().getConnectionAttempts() % NUM_DOMAIN_SERVER_PINGS_BEFORE_RESET == 0) {
                // if we have then nullify the domain handler's network peer and send a fresh ICE heartbeat
                qCDebug(networking) << "No ping replies received from domain-server with ID"
                    << uuidStringWithoutCurlyBraces(_domainHandler.getICEClientID()) << "-" << "re-sending ICE query.";

                _domainHandler.getICEPeer().softReset();
                handleICEConnectionToDomainServer();

                return;
            }
        }

        flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendPingsToDS);

        // send the ping packet to the local and public sockets for this node
        auto localPingPacket = constructICEPingPacket(PingType::Local);
        sendPacket(localPingPacket, _domainHandler.getICEPeer().getLocalSocket());

        auto publicPingPacket = constructICEPingPacket(PingType::Public);
        sendPacket(publicPingPacket, _domainHandler.getICEPeer().getPublicSocket());

        _domainHandler.getICEPeer().incrementConnectionAttempts();
    }
}

int NodeList::processDomainServerList(const QByteArray& packet) {
    // this is a packet from the domain server, reset the count of un-replied check-ins
    _numNoReplyDomainCheckIns = 0;

    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::ReceiveDSList);

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

    // TODO: when fixing this read these are actually chars now, not bools
    bool thisNodeCanAdjustLocks;
    packetStream >> thisNodeCanAdjustLocks;
    setThisNodeCanAdjustLocks(thisNodeCanAdjustLocks);

    bool thisNodeCanRez;
    packetStream >> thisNodeCanRez;
    setThisNodeCanRez(thisNodeCanRez);

    // pull each node in the packet
    while (packetStream.device()->pos() < packet.size()) {
        parseNodeFromPacketStream(packetStream);
    }

    return readNodes;
}

void NodeList::processDomainServerAddedNode(const QByteArray& packet) {
    // setup a QDataStream, skip the header
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));

    // use our shared method to pull out the new node
    parseNodeFromPacketStream(packetStream);
}

void NodeList::parseNodeFromPacketStream(QDataStream& packetStream) {
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

    packetStream >> connectionUUID;

    SharedNodePointer node = addOrUpdateNode(nodeUUID, nodeType, nodePublicSocket,
                                             nodeLocalSocket, canAdjustLocks, canRez,
                                             connectionUUID);
}

void NodeList::sendAssignment(Assignment& assignment) {

    PacketType::Value assignmentPacketType = assignment.getCommand() == Assignment::CreateCommand
        ? PacketTypeCreateAssignment
        : PacketTypeRequestAssignment;

    QByteArray packet = byteArrayWithPopulatedHeader(assignmentPacketType);
    QDataStream packetStream(&packet, QIODevice::Append);

    packetStream << assignment;

    _nodeSocket.writeDatagram(packet, _assignmentServerSocket.getAddress(), _assignmentServerSocket.getPort());
}

void NodeList::pingPunchForInactiveNode(const SharedNodePointer& node) {
    if (node->getType() == NodeType::AudioMixer) {
        flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendAudioPing);
    }

    // every second we're trying to ping this node and we're not getting anywhere - debug that out
    const int NUM_DEBUG_CONNECTION_ATTEMPTS = 1000 / (UDP_PUNCH_PING_INTERVAL_MS);

    if (node->getConnectionAttempts() > 0 && node->getConnectionAttempts() % NUM_DEBUG_CONNECTION_ATTEMPTS == 0) {
        qCDebug(networking) << "No response to UDP hole punch pings for node" << node->getUUID() << "in last second.";
    }

    // send the ping packet to the local and public sockets for this node
    auto localPingPacket = constructPingPacket(PingType::Local);
    sendPacket(localPingPacket, node, node->getLocalSocket());

    auto publicPingPacket = constructPingPacket(PingType::Public);
    sendPacket(publicPingPacket, node, node->getPublicSocket());

    if (!node->getSymmetricSocket().isNull()) {
        auto symmetricPingPacket = constructPingPacket(PingType::Symmetric);
        sendPacket(symmetricPingPacket, node, node->getSymmetricSocket());
    }

    node->incrementConnectionAttempts();
}

void NodeList::startNodeHolePunch(const SharedNodePointer& node) {
    // connect to the correct signal on this node so we know when to ping it
    connect(node.data(), &Node::pingTimerTimeout, this, &NodeList::handleNodePingTimeout);

    // start the ping timer for this node
    node->startPingTimer();

    // ping this node immediately
    pingPunchForInactiveNode(node);
}

void NodeList::handleNodePingTimeout() {
    Node* senderNode = qobject_cast<Node*>(sender());
    if (senderNode) {
        SharedNodePointer sharedNode = nodeWithUUID(senderNode->getUUID());

        if (sharedNode && !sharedNode->getActiveSocket()) {
            pingPunchForInactiveNode(sharedNode);
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

    if (sendingNode->getType() == NodeType::AudioMixer) {
       flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetAudioMixerSocket);
    }
}
