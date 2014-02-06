//
//  NodeList.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtNetwork/QHostInfo>

#include "Assignment.h"
#include "HifiSockAddr.h"
#include "Logging.h"
#include "NodeList.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "UUID.h"

const char SOLO_NODE_TYPES[2] = {
    NodeType::AvatarMixer,
    NodeType::AudioMixer
};

const QString DEFAULT_DOMAIN_HOSTNAME = "alpha.highfidelity.io";
const unsigned short DEFAULT_DOMAIN_SERVER_PORT = 40102;

NodeList* NodeList::_sharedInstance = NULL;

NodeList* NodeList::createInstance(char ownerType, unsigned short int socketListenPort) {
    if (!_sharedInstance) {
        NodeType::init();
        
        _sharedInstance = new NodeList(ownerType, socketListenPort);

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


NodeList::NodeList(char newOwnerType, unsigned short int newSocketListenPort) :
    _nodeHash(),
    _nodeHashMutex(QMutex::Recursive),
    _domainHostname(DEFAULT_DOMAIN_HOSTNAME),
    _domainSockAddr(HifiSockAddr(QHostAddress::Null, DEFAULT_DOMAIN_SERVER_PORT)),
    _nodeSocket(this),
    _ownerType(newOwnerType),
    _nodeTypesOfInterest(),
    _sessionUUID(),
    _numNoReplyDomainCheckIns(0),
    _assignmentServerSocket(),
    _publicSockAddr(),
    _hasCompletedInitialSTUNFailure(false),
    _stunRequestsSinceSuccess(0)
{
    _nodeSocket.bind(QHostAddress::AnyIPv4, newSocketListenPort);
    qDebug() << "NodeList socket is listening on" << _nodeSocket.localPort();
}


NodeList::~NodeList() {
    clear();
}

void NodeList::setDomainHostname(const QString& domainHostname) {

    if (domainHostname != _domainHostname) {
        int colonIndex = domainHostname.indexOf(':');

        if (colonIndex > 0) {
            // the user has included a custom DS port with the hostname

            // the new hostname is everything up to the colon
            _domainHostname = domainHostname.left(colonIndex);

            // grab the port by reading the string after the colon
            _domainSockAddr.setPort(atoi(domainHostname.mid(colonIndex + 1, domainHostname.size()).toLocal8Bit().constData()));

            qDebug() << "Updated hostname to" << _domainHostname << "and port to" << _domainSockAddr.getPort();

        } else {
            // no port included with the hostname, simply set the member variable and reset the domain server port to default
            _domainHostname = domainHostname;
            _domainSockAddr.setPort(DEFAULT_DOMAIN_SERVER_PORT);
        }

        // clear the NodeList so nodes from this domain are killed
        clear();

        // reset our _domainIP to the null address so that a lookup happens on next check in
        _domainSockAddr.setAddress(QHostAddress::Null);
        emit domainChanged(_domainHostname);
    }
}

void NodeList::timePingReply(const QByteArray& packet) {
    QUuid nodeUUID;
    deconstructPacketHeader(packet, nodeUUID);
    
    SharedNodePointer matchingNode = nodeWithUUID(nodeUUID);
    
    if (matchingNode) {
        QDataStream packetStream(packet);
        packetStream.skipRawData(numBytesForPacketHeader(packet));
        
        quint64 ourOriginalTime, othersReplyTime;
        
        packetStream >> ourOriginalTime >> othersReplyTime;
        
        quint64 now = usecTimestampNow();
        int pingTime = now - ourOriginalTime;
        int oneWayFlightTime = pingTime / 2; // half of the ping is our one way flight
        
        // The other node's expected time should be our original time plus the one way flight time
        // anything other than that is clock skew
        quint64 othersExprectedReply = ourOriginalTime + oneWayFlightTime;
        int clockSkew = othersReplyTime - othersExprectedReply;
        
        matchingNode->setPingMs(pingTime / 1000);
        matchingNode->setClockSkewUsec(clockSkew);
        
        const bool wantDebug = false;
        
        if (wantDebug) {
            qDebug() << "PING_REPLY from node " << *matchingNode << "\n" <<
            "                     now: " << now << "\n" <<
            "                 ourTime: " << ourOriginalTime << "\n" <<
            "                pingTime: " << pingTime << "\n" <<
            "        oneWayFlightTime: " << oneWayFlightTime << "\n" <<
            "         othersReplyTime: " << othersReplyTime << "\n" <<
            "    othersExprectedReply: " << othersExprectedReply << "\n" <<
            "               clockSkew: " << clockSkew;
        }
    }
}

void NodeList::processNodeData(const HifiSockAddr& senderSockAddr, const QByteArray& packet) {
    switch (packetTypeForPacket(packet)) {
        case PacketTypeDomainList: {
            // only process the DS if this is our current domain server
            if (_domainSockAddr == senderSockAddr) {
                processDomainServerList(packet);
            }

            break;
        }
        case PacketTypePing: {
            // send back a reply
            QByteArray replyPacket = constructPingReplyPacket(packet);
            _nodeSocket.writeDatagram(replyPacket, senderSockAddr.getAddress(), senderSockAddr.getPort());
            break;
        }
        case PacketTypePingReply: {
            // activate the appropriate socket for this node, if not yet updated
            activateSocketFromNodeCommunication(senderSockAddr);

            // set the ping time for this node for stat collection
            timePingReply(packet);
            break;
        }
        case PacketTypeStunResponse: {
            // a STUN packet begins with 00, we've checked the second zero with packetVersionMatch
            // pass it along so it can be processed into our public address and port
            processSTUNResponse(packet);
            break;
        }
        default:
            break;
    }
}

int NodeList::updateNodeWithData(Node *node, const HifiSockAddr& senderSockAddr, const QByteArray& packet) {
    QMutexLocker locker(&node->getMutex());

    node->setLastHeardMicrostamp(usecTimestampNow());

    if (!senderSockAddr.isNull() && !node->getActiveSocket()) {
        if (senderSockAddr == node->getPublicSocket()) {
            node->activatePublicSocket();
        } else if (senderSockAddr == node->getLocalSocket()) {
            node->activateLocalSocket();
        }
    }

    if (node->getActiveSocket() || senderSockAddr.isNull()) {
        node->recordBytesReceived(packet.size());

        if (!node->getLinkedData() && linkedDataCreateCallback) {
            linkedDataCreateCallback(node);
        }

        int numParsedBytes = node->getLinkedData()->parseData(packet);
        return numParsedBytes;
    } else {
        // we weren't able to match the sender address to the address we have for this node, unlock and don't parse
        return 0;
    }
}

SharedNodePointer NodeList::nodeWithAddress(const HifiSockAddr &senderSockAddr) {
    // naively returns the first node that has a matching active HifiSockAddr
    // note that there can be multiple nodes that have a matching active socket, so this isn't a good way to uniquely identify
    foreach (const SharedNodePointer& node, getNodeHash()) {
        if (node->getActiveSocket() && *node->getActiveSocket() == senderSockAddr) {
            return node;
        }
    }

    return SharedNodePointer();
}

SharedNodePointer NodeList::nodeWithUUID(const QUuid& nodeUUID) {
    QMutexLocker locker(&_nodeHashMutex);
    return _nodeHash.value(nodeUUID);
}

NodeHash NodeList::getNodeHash() {
    QMutexLocker locker(&_nodeHashMutex);
    return NodeHash(_nodeHash);
}

void NodeList::clear() {
    qDebug() << "Clearing the NodeList. Deleting all nodes in list.";
    
    QMutexLocker locker(&_nodeHashMutex);

    NodeHash::iterator nodeItem = _nodeHash.begin();

    // iterate the nodes in the list
    while (nodeItem != _nodeHash.end()) {
        nodeItem = killNodeAtHashIterator(nodeItem);
    }
}

void NodeList::reset() {
    clear();
    _numNoReplyDomainCheckIns = 0;

    _nodeTypesOfInterest.clear();

    // refresh the owner UUID
    _sessionUUID = QUuid::createUuid();
}

void NodeList::addNodeTypeToInterestSet(NodeType_t nodeTypeToAdd) {
    _nodeTypesOfInterest << nodeTypeToAdd;
}

void NodeList::addSetOfNodeTypesToNodeInterestSet(const NodeSet& setOfNodeTypes) {
    _nodeTypesOfInterest.unite(setOfNodeTypes);
}

const uint32_t RFC_5389_MAGIC_COOKIE = 0x2112A442;
const int NUM_BYTES_STUN_HEADER = 20;
const int NUM_STUN_REQUESTS_BEFORE_FALLBACK = 5;

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
    unsigned char transactionID[NUM_TRANSACTION_ID_BYTES];
    loadRandomIdentifier(transactionID, NUM_TRANSACTION_ID_BYTES);
    memcpy(stunRequestPacket + packetIndex, &transactionID, sizeof(transactionID));

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
                memcpy(&addressFamily, packet.data(), sizeof(addressFamily));

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

void NodeList::killNodeWithUUID(const QUuid& nodeUUID) {
    QMutexLocker locker(&_nodeHashMutex);
    
    NodeHash::iterator nodeItemToKill = _nodeHash.find(nodeUUID);
    if (nodeItemToKill != _nodeHash.end()) {
        killNodeAtHashIterator(nodeItemToKill);
    }
}

NodeHash::iterator NodeList::killNodeAtHashIterator(NodeHash::iterator& nodeItemToKill) {
    qDebug() << "Killed" << *nodeItemToKill.value();
    emit nodeKilled(nodeItemToKill.value());
    return _nodeHash.erase(nodeItemToKill);
}

void NodeList::processKillNode(const QByteArray& dataByteArray) {
    // read the node id
    QUuid nodeUUID = QUuid::fromRfc4122(dataByteArray.mid(numBytesForPacketHeader(dataByteArray), NUM_BYTES_RFC4122_UUID));

    // kill the node with this UUID, if it exists
    killNodeWithUUID(nodeUUID);
}

void NodeList::sendDomainServerCheckIn() {
    static bool printedDomainServerIP = false;

    //  Lookup the IP address of the domain server if we need to
    if (_domainSockAddr.getAddress().isNull()) {
        qDebug("Looking up DS hostname %s.", _domainHostname.toLocal8Bit().constData());
        QHostInfo domainServerHostInfo = QHostInfo::fromName(_domainHostname);

        for (int i = 0; i < domainServerHostInfo.addresses().size(); i++) {
            if (domainServerHostInfo.addresses()[i].protocol() == QAbstractSocket::IPv4Protocol) {
                _domainSockAddr.setAddress(domainServerHostInfo.addresses()[i]);
                qDebug("DS at %s is at %s", _domainHostname.toLocal8Bit().constData(),
                       _domainSockAddr.getAddress().toString().toLocal8Bit().constData());

                printedDomainServerIP = true;

                break;
            }

            // if we got here without a break out of the for loop then we failed to lookup the address
            if (i == domainServerHostInfo.addresses().size() - 1) {
                qDebug("Failed domain server lookup");
            }
        }
    } else if (!printedDomainServerIP) {
        qDebug("Domain Server IP: %s", _domainSockAddr.getAddress().toString().toLocal8Bit().constData());
        printedDomainServerIP = true;
    }

    if (_publicSockAddr.isNull() && !_hasCompletedInitialSTUNFailure) {
        // we don't know our public socket and we need to send it to the domain server
        // send a STUN request to figure it out
        sendSTUNRequest();
    } else {
        // construct the DS check in packet if we need to

        // check in packet has header, optional UUID, node type, port, IP, node types of interest, null termination
        QByteArray domainServerPacket = byteArrayWithPopluatedHeader(PacketTypeDomainListRequest);
        QDataStream packetStream(&domainServerPacket, QIODevice::Append);

        // pack our data to send to the domain-server
        packetStream << _ownerType << _publicSockAddr
            << HifiSockAddr(QHostAddress(getHostOrderLocalAddress()), _nodeSocket.localPort())
            << (quint8) _nodeTypesOfInterest.size();
        
        // copy over the bytes for node types of interest, if required
        foreach (NodeType_t nodeTypeOfInterest, _nodeTypesOfInterest) {
            packetStream << nodeTypeOfInterest;
        }
        
        _nodeSocket.writeDatagram(domainServerPacket, _domainSockAddr.getAddress(), _domainSockAddr.getPort());
        const int NUM_DOMAIN_SERVER_CHECKINS_PER_STUN_REQUEST = 5;
        static unsigned int numDomainCheckins = 0;

        // send a STUN request every Nth domain server check in so we update our public socket, if required
        if (numDomainCheckins++ % NUM_DOMAIN_SERVER_CHECKINS_PER_STUN_REQUEST == 0) {
            sendSTUNRequest();
        }

        // increment the count of un-replied check-ins
        _numNoReplyDomainCheckIns++;
    }
}

void NodeList::setSessionUUID(const QUuid& sessionUUID) {
    QUuid oldUUID = _sessionUUID;
    _sessionUUID = sessionUUID;
    
    if (sessionUUID != oldUUID) {
        qDebug() << "NodeList UUID changed from" << oldUUID << "to" << _sessionUUID;
        emit uuidChanged(sessionUUID);
    }
}

int NodeList::processDomainServerList(const QByteArray& packet) {
    // this is a packet from the domain server, reset the count of un-replied check-ins
    _numNoReplyDomainCheckIns = 0;

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
            nodePublicSocket.setAddress(_domainSockAddr.getAddress());
        }

        SharedNodePointer node = addOrUpdateNode(nodeUUID, nodeType, nodePublicSocket, nodeLocalSocket);
        
        packetStream >> connectionUUID;
        node->setConnectionSecret(connectionUUID);
    }

    return readNodes;
}

void NodeList::sendAssignment(Assignment& assignment) {
    
    PacketType assignmentPacketType = assignment.getCommand() == Assignment::CreateCommand
        ? PacketTypeCreateAssignment
        : PacketTypeRequestAssignment;
    
    QByteArray packet = byteArrayWithPopluatedHeader(assignmentPacketType);
    QDataStream packetStream(&packet, QIODevice::Append);
    
    packetStream << assignment;

    static HifiSockAddr DEFAULT_ASSIGNMENT_SOCKET(DEFAULT_ASSIGNMENT_SERVER_HOSTNAME, DEFAULT_DOMAIN_SERVER_PORT);

    const HifiSockAddr* assignmentServerSocket = _assignmentServerSocket.isNull()
        ? &DEFAULT_ASSIGNMENT_SOCKET
        : &_assignmentServerSocket;

    _nodeSocket.writeDatagram(packet, assignmentServerSocket->getAddress(), assignmentServerSocket->getPort());
}

QByteArray NodeList::constructPingPacket() {
    QByteArray pingPacket = byteArrayWithPopluatedHeader(PacketTypePing);
    
    QDataStream packetStream(&pingPacket, QIODevice::Append);
    packetStream << usecTimestampNow();
    
    return pingPacket;
}

QByteArray NodeList::constructPingReplyPacket(const QByteArray& pingPacket) {
    QDataStream pingPacketStream(pingPacket);
    pingPacketStream.skipRawData(numBytesForPacketHeader(pingPacket));
    
    quint64 timeFromOriginalPing;
    pingPacketStream >> timeFromOriginalPing;
    
    QByteArray replyPacket = byteArrayWithPopluatedHeader(PacketTypePingReply);
    QDataStream packetStream(&replyPacket, QIODevice::Append);
    
    packetStream << timeFromOriginalPing << usecTimestampNow();
    
    return replyPacket;
}

void NodeList::pingPublicAndLocalSocketsForInactiveNode(Node* node) {
    QByteArray pingPacket = constructPingPacket();

    // send the ping packet to the local and public sockets for this node
    _nodeSocket.writeDatagram(pingPacket, node->getLocalSocket().getAddress(), node->getLocalSocket().getPort());
    _nodeSocket.writeDatagram(pingPacket, node->getPublicSocket().getAddress(), node->getPublicSocket().getPort());
}

SharedNodePointer NodeList::addOrUpdateNode(const QUuid& uuid, char nodeType,
                                            const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket) {
    _nodeHashMutex.lock();
    
    SharedNodePointer matchingNode = _nodeHash.value(uuid);
    
    if (!matchingNode) {
        // we didn't have this node, so add them
        Node* newNode = new Node(uuid, nodeType, publicSocket, localSocket);
        SharedNodePointer newNodeSharedPointer(newNode, &QObject::deleteLater);

        _nodeHash.insert(newNode->getUUID(), newNodeSharedPointer);

        _nodeHashMutex.unlock();
        
        qDebug() << "Added" << *newNode;

        emit nodeAdded(newNodeSharedPointer);

        return newNodeSharedPointer;
    } else {
        _nodeHashMutex.unlock();
        
        QMutexLocker locker(&matchingNode->getMutex());

        if (matchingNode->getType() == NodeType::AudioMixer ||
            matchingNode->getType() == NodeType::VoxelServer ||
            matchingNode->getType() == NodeType::MetavoxelServer) {
            // until the Audio class also uses our nodeList, we need to update
            // the lastRecvTimeUsecs for the audio mixer so it doesn't get killed and re-added continously
            matchingNode->setLastHeardMicrostamp(usecTimestampNow());
        }

        // check if we need to change this node's public or local sockets
        if (publicSocket != matchingNode->getPublicSocket()) {
            matchingNode->setPublicSocket(publicSocket);
            qDebug() << "Public socket change for node" << *matchingNode;
        }

        if (localSocket != matchingNode->getLocalSocket()) {
            matchingNode->setLocalSocket(localSocket);
            qDebug() << "Local socket change for node" << *matchingNode;
        }
        // we had this node already, do nothing for now
        return matchingNode;
    }
}

unsigned NodeList::broadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes) {
    unsigned n = 0;

    foreach (const SharedNodePointer& node, getNodeHash()) {
        // only send to the NodeTypes we are asked to send to.
        if (destinationNodeTypes.contains(node->getType())) {
            if (getNodeActiveSocketOrPing(node.data())) {
                // we know which socket is good for this node, send there
                _nodeSocket.writeDatagram(packet, node->getActiveSocket()->getAddress(), node->getActiveSocket()->getPort());
                ++n;
            }
        }
    }

    return n;
}

void NodeList::pingInactiveNodes() {
    foreach (const SharedNodePointer& node, getNodeHash()) {
        if (!node->getActiveSocket()) {
            // we don't have an active link to this node, ping it to set that up
            pingPublicAndLocalSocketsForInactiveNode(node.data());
        }
    }
}

const HifiSockAddr* NodeList::getNodeActiveSocketOrPing(Node* node) {
    if (node->getActiveSocket()) {
        return node->getActiveSocket();
    } else {
        pingPublicAndLocalSocketsForInactiveNode(node);
        return NULL;
    }
}

void NodeList::activateSocketFromNodeCommunication(const HifiSockAddr& nodeAddress) {

    foreach (const SharedNodePointer& node, _nodeHash) {
        if (!node->getActiveSocket()) {
            // check both the public and local addresses for each node to see if we find a match
            // prioritize the private address so that we prune erroneous local matches
            if (node->getPublicSocket() ==  nodeAddress) {
                node->activatePublicSocket();
                break;
            } else if (node->getLocalSocket() == nodeAddress) {
                node->activateLocalSocket();
                break;
            }
        }
    }
}

SharedNodePointer NodeList::soloNodeOfType(char nodeType) {

    if (memchr(SOLO_NODE_TYPES, nodeType, sizeof(SOLO_NODE_TYPES)) != NULL) {
        foreach (const SharedNodePointer& node, getNodeHash()) {
            if (node->getType() == nodeType) {
                return node;
            }
        }
    }
    return SharedNodePointer();
}

void NodeList::removeSilentNodes() {

    _nodeHashMutex.lock();
    
    NodeHash::iterator nodeItem = _nodeHash.begin();

    while (nodeItem != _nodeHash.end()) {
        SharedNodePointer node = nodeItem.value();

        node->getMutex().lock();

        if ((usecTimestampNow() - node->getLastHeardMicrostamp()) > NODE_SILENCE_THRESHOLD_USECS) {
            // call our private method to kill this node (removes it and emits the right signal)
            nodeItem = killNodeAtHashIterator(nodeItem);
        } else {
            // we didn't kill this node, push the iterator forwards
            ++nodeItem;
        }
        
        node->getMutex().unlock();
    }
    
    _nodeHashMutex.unlock();
}

const QString QSETTINGS_GROUP_NAME = "NodeList";
const QString DOMAIN_SERVER_SETTING_KEY = "domainServerHostname";

void NodeList::loadData(QSettings *settings) {
    settings->beginGroup(DOMAIN_SERVER_SETTING_KEY);

    QString domainServerHostname = settings->value(DOMAIN_SERVER_SETTING_KEY).toString();

    if (domainServerHostname.size() > 0) {
        _domainHostname = domainServerHostname;
        emit domainChanged(_domainHostname);
    }

    settings->endGroup();
}

void NodeList::saveData(QSettings* settings) {
    settings->beginGroup(DOMAIN_SERVER_SETTING_KEY);

    if (_domainHostname != DEFAULT_DOMAIN_HOSTNAME) {
        // the user is using a different hostname, store it
        settings->setValue(DOMAIN_SERVER_SETTING_KEY, QVariant(_domainHostname));
    } else {
        // the user has switched back to default, remove the current setting
        settings->remove(DOMAIN_SERVER_SETTING_KEY);
    }

    settings->endGroup();
}
