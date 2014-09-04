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

NodeList* NodeList::_sharedInstance = NULL;

NodeList* NodeList::createInstance(char ownerType, unsigned short socketListenPort, unsigned short dtlsPort) {
    if (!_sharedInstance) {
        NodeType::init();
        
        _sharedInstance = new NodeList(ownerType, socketListenPort, dtlsPort);
        LimitedNodeList::_sharedInstance = _sharedInstance;

        // register the SharedNodePointer meta-type for signals/slots
        qRegisterMetaType<SharedNodePointer>();
    } else {
        qDebug("NodeList createInstance called with existing instance.");
    }

    return _sharedInstance;
}

NodeList* NodeList::getInstance() {
    if (!_sharedInstance) {
        qDebug("NodeList getInstance called before call to createInstance. Returning NULL pointer.");
    }

    return _sharedInstance;
}

NodeList::NodeList(char newOwnerType, unsigned short socketListenPort, unsigned short dtlsListenPort) :
    LimitedNodeList(socketListenPort, dtlsListenPort),
    _ownerType(newOwnerType),
    _nodeTypesOfInterest(),
    _domainHandler(this),
    _numNoReplyDomainCheckIns(0),
    _assignmentServerSocket(),
    _publicSockAddr(),
    _hasCompletedInitialSTUNFailure(false),
    _stunRequestsSinceSuccess(0)
{
    // clear our NodeList when the domain changes
    connect(&_domainHandler, &DomainHandler::hostnameChanged, this, &NodeList::reset);
    
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

const uint32_t RFC_5389_MAGIC_COOKIE = 0x2112A442;
const int NUM_BYTES_STUN_HEADER = 20;
const unsigned int NUM_STUN_REQUESTS_BEFORE_FALLBACK = 5;

void NodeList::sendSTUNRequest() {
    const char STUN_SERVER_HOSTNAME[] = "stun.highfidelity.io";
    const unsigned short STUN_SERVER_PORT = 3478;

    unsigned char stunRequestPacket[NUM_BYTES_STUN_HEADER];

    int packetIndex = 0;

    const uint32_t RFC_5389_MAGIC_COOKIE_NETWORK_ORDER = htonl(RFC_5389_MAGIC_COOKIE);

    // leading zeros + message type
    const uint16_t REQUEST_MESSAGE_TYPE = htons(0x0001);
    memcpy(stunRequestPacket + packetIndex, &REQUEST_MESSAGE_TYPE, sizeof(REQUEST_MESSAGE_TYPE));
    packetIndex += sizeof(REQUEST_MESSAGE_TYPE);

    // message length (no additional attributes are included)
    uint16_t messageLength = 0;
    memcpy(stunRequestPacket + packetIndex, &messageLength, sizeof(messageLength));
    packetIndex += sizeof(messageLength);

    memcpy(stunRequestPacket + packetIndex, &RFC_5389_MAGIC_COOKIE_NETWORK_ORDER, sizeof(RFC_5389_MAGIC_COOKIE_NETWORK_ORDER));
    packetIndex += sizeof(RFC_5389_MAGIC_COOKIE_NETWORK_ORDER);

    // transaction ID (random 12-byte unsigned integer)
    const uint NUM_TRANSACTION_ID_BYTES = 12;
    QUuid randomUUID = QUuid::createUuid();
    memcpy(stunRequestPacket + packetIndex, randomUUID.toRfc4122().data(), NUM_TRANSACTION_ID_BYTES);

    // lookup the IP for the STUN server
    static HifiSockAddr stunSockAddr(STUN_SERVER_HOSTNAME, STUN_SERVER_PORT);

    if (!_hasCompletedInitialSTUNFailure) {
        qDebug("Sending intial stun request to %s", stunSockAddr.getAddress().toString().toLocal8Bit().constData());
    }

    _nodeSocket.writeDatagram((char*) stunRequestPacket, sizeof(stunRequestPacket),
                              stunSockAddr.getAddress(), stunSockAddr.getPort());

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

void NodeList::processSTUNResponse(const QByteArray& packet) {
    // check the cookie to make sure this is actually a STUN response
    // and read the first attribute and make sure it is a XOR_MAPPED_ADDRESS
    const int NUM_BYTES_MESSAGE_TYPE_AND_LENGTH = 4;
    const uint16_t XOR_MAPPED_ADDRESS_TYPE = htons(0x0020);

    const uint32_t RFC_5389_MAGIC_COOKIE_NETWORK_ORDER = htonl(RFC_5389_MAGIC_COOKIE);

    int attributeStartIndex = NUM_BYTES_STUN_HEADER;

    if (memcmp(packet.data() + NUM_BYTES_MESSAGE_TYPE_AND_LENGTH,
               &RFC_5389_MAGIC_COOKIE_NETWORK_ORDER,
               sizeof(RFC_5389_MAGIC_COOKIE_NETWORK_ORDER)) == 0) {

        // enumerate the attributes to find XOR_MAPPED_ADDRESS_TYPE
        while (attributeStartIndex < packet.size()) {
            if (memcmp(packet.data() + attributeStartIndex, &XOR_MAPPED_ADDRESS_TYPE, sizeof(XOR_MAPPED_ADDRESS_TYPE)) == 0) {
                const int NUM_BYTES_STUN_ATTR_TYPE_AND_LENGTH = 4;
                const int NUM_BYTES_FAMILY_ALIGN = 1;
                const uint8_t IPV4_FAMILY_NETWORK_ORDER = htons(0x01) >> 8;

                // reset the number of failed STUN requests since last success
                _stunRequestsSinceSuccess = 0;

                int byteIndex = attributeStartIndex + NUM_BYTES_STUN_ATTR_TYPE_AND_LENGTH + NUM_BYTES_FAMILY_ALIGN;
                
                uint8_t addressFamily = 0;
                memcpy(&addressFamily, packet.data() + byteIndex, sizeof(addressFamily));

                byteIndex += sizeof(addressFamily);

                if (addressFamily == IPV4_FAMILY_NETWORK_ORDER) {
                    // grab the X-Port
                    uint16_t xorMappedPort = 0;
                    memcpy(&xorMappedPort, packet.data() + byteIndex, sizeof(xorMappedPort));

                    uint16_t newPublicPort = ntohs(xorMappedPort) ^ (ntohl(RFC_5389_MAGIC_COOKIE_NETWORK_ORDER) >> 16);

                    byteIndex += sizeof(xorMappedPort);

                    // grab the X-Address
                    uint32_t xorMappedAddress = 0;
                    memcpy(&xorMappedAddress, packet.data() + byteIndex, sizeof(xorMappedAddress));

                    uint32_t stunAddress = ntohl(xorMappedAddress) ^ ntohl(RFC_5389_MAGIC_COOKIE_NETWORK_ORDER);

                    QHostAddress newPublicAddress = QHostAddress(stunAddress);

                    if (newPublicAddress != _publicSockAddr.getAddress() || newPublicPort != _publicSockAddr.getPort()) {
                        _publicSockAddr = HifiSockAddr(newPublicAddress, newPublicPort);

                        qDebug("New public socket received from STUN server is %s:%hu",
                               _publicSockAddr.getAddress().toString().toLocal8Bit().constData(),
                               _publicSockAddr.getPort());

                    }

                    _hasCompletedInitialSTUNFailure = true;

                    break;
                }
            } else {
                // push forward attributeStartIndex by the length of this attribute
                const int NUM_BYTES_ATTRIBUTE_TYPE = 2;

                uint16_t attributeLength = 0;
                memcpy(&attributeLength, packet.data() + attributeStartIndex + NUM_BYTES_ATTRIBUTE_TYPE,
                       sizeof(attributeLength));
                attributeLength = ntohs(attributeLength);

                attributeStartIndex += NUM_BYTES_MESSAGE_TYPE_AND_LENGTH + attributeLength;
            }
        }
    }
}

void NodeList::sendDomainServerCheckIn() {
    if (_publicSockAddr.isNull() && !_hasCompletedInitialSTUNFailure) {
        // we don't know our public socket and we need to send it to the domain server
        // send a STUN request to figure it out
        sendSTUNRequest();
    } else if (!_domainHandler.getIP().isNull()) {

        bool isUsingDTLS = false;
        
        PacketType domainPacketType = !_domainHandler.isConnected()
            ? PacketTypeDomainConnectRequest : PacketTypeDomainListRequest;
        
        if (!_domainHandler.isConnected()) {
            qDebug() << "Sending connect request to domain-server at" << _domainHandler.getHostname();
        }
        
        // construct the DS check in packet
        QUuid packetUUID = _sessionUUID;
        
        if (!_domainHandler.getAssignmentUUID().isNull() && domainPacketType == PacketTypeDomainConnectRequest) {
            // this is a connect request and we're an assigned node
            // so set our packetUUID as the assignment UUID
            packetUUID = _domainHandler.getAssignmentUUID();
        }
        
        QByteArray domainServerPacket = byteArrayWithPopulatedHeader(domainPacketType, packetUUID);
        QDataStream packetStream(&domainServerPacket, QIODevice::Append);
        
        // pack our data to send to the domain-server
        packetStream << _ownerType << _publicSockAddr
            << HifiSockAddr(QHostAddress(getHostOrderLocalAddress()), _nodeSocket.localPort())
            << (quint8) _nodeTypesOfInterest.size();
        
        // copy over the bytes for node types of interest, if required
        foreach (NodeType_t nodeTypeOfInterest, _nodeTypesOfInterest) {
            packetStream << nodeTypeOfInterest;
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

int NodeList::processDomainServerList(const QByteArray& packet) {
    // this is a packet from the domain server, reset the count of un-replied check-ins
    _numNoReplyDomainCheckIns = 0;
    
    // if this was the first domain-server list from this domain, we've now connected
    _domainHandler.setIsConnected(true);

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

    static HifiSockAddr DEFAULT_ASSIGNMENT_SOCKET(DEFAULT_ASSIGNMENT_SERVER_HOSTNAME, DEFAULT_DOMAIN_SERVER_PORT);

    const HifiSockAddr* assignmentServerSocket = _assignmentServerSocket.isNull()
        ? &DEFAULT_ASSIGNMENT_SOCKET
        : &_assignmentServerSocket;

    _nodeSocket.writeDatagram(packet, assignmentServerSocket->getAddress(), assignmentServerSocket->getPort());
}

QByteArray NodeList::constructPingPacket(PingType_t pingType) {
    QByteArray pingPacket = byteArrayWithPopulatedHeader(PacketTypePing);
    
    QDataStream packetStream(&pingPacket, QIODevice::Append);
    
    packetStream << pingType;
    packetStream << usecTimestampNow();
    
    return pingPacket;
}

QByteArray NodeList::constructPingReplyPacket(const QByteArray& pingPacket) {
    QDataStream pingPacketStream(pingPacket);
    pingPacketStream.skipRawData(numBytesForPacketHeader(pingPacket));
    
    PingType_t typeFromOriginalPing;
    pingPacketStream >> typeFromOriginalPing;
    
    quint64 timeFromOriginalPing;
    pingPacketStream >> timeFromOriginalPing;
    
    QByteArray replyPacket = byteArrayWithPopulatedHeader(PacketTypePingReply);
    QDataStream packetStream(&replyPacket, QIODevice::Append);
    
    packetStream << typeFromOriginalPing << timeFromOriginalPing << usecTimestampNow();
    
    return replyPacket;
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

const QString QSETTINGS_GROUP_NAME = "NodeList";
const QString DOMAIN_SERVER_SETTING_KEY = "domainServerHostname";

void NodeList::loadData(QSettings *settings) {
    settings->beginGroup(DOMAIN_SERVER_SETTING_KEY);

    QString domainServerHostname = settings->value(DOMAIN_SERVER_SETTING_KEY).toString();

    if (domainServerHostname.size() > 0) {
        _domainHandler.setHostname(domainServerHostname);
    } else {
        _domainHandler.setHostname(DEFAULT_DOMAIN_HOSTNAME);
    }
    
    settings->endGroup();
}

void NodeList::saveData(QSettings* settings) {
    settings->beginGroup(DOMAIN_SERVER_SETTING_KEY);

    if (_domainHandler.getHostname() != DEFAULT_DOMAIN_HOSTNAME) {
        // the user is using a different hostname, store it
        settings->setValue(DOMAIN_SERVER_SETTING_KEY, QVariant(_domainHandler.getHostname()));
    } else {
        // the user has switched back to default, remove the current setting
        settings->remove(DOMAIN_SERVER_SETTING_KEY);
    }

    settings->endGroup();
}
