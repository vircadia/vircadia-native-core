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

const char SOLO_NODE_TYPES[2] = {
    NodeType::AvatarMixer,
    NodeType::AudioMixer
};

const QUrl DEFAULT_NODE_AUTH_URL = QUrl("https://data-web.highfidelity.io");

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
    _nodeSocket(this),
    _ownerType(newOwnerType),
    _nodeTypesOfInterest(),
    _sessionUUID(),
    _numNoReplyDomainCheckIns(0),
    _assignmentServerSocket(),
    _publicSockAddr(),
    _hasCompletedInitialSTUNFailure(false),
    _stunRequestsSinceSuccess(0),
    _numCollectedPackets(0),
    _numCollectedBytes(0),
    _packetStatTimer()
{
    _nodeSocket.bind(QHostAddress::AnyIPv4, newSocketListenPort);
    qDebug() << "NodeList socket is listening on" << _nodeSocket.localPort();
    
    // clear our NodeList when the domain changes
    connect(&_domainInfo, &DomainInfo::hostnameChanged, this, &NodeList::reset);
    
    // clear our NodeList when logout is requested
    connect(&AccountManager::getInstance(), &AccountManager::logoutComplete , this, &NodeList::reset);
    
    _packetStatTimer.start();
}

bool NodeList::packetVersionAndHashMatch(const QByteArray& packet) {
    PacketType checkType = packetTypeForPacket(packet);
    if (packet[1] != versionForPacketType(checkType)
        && checkType != PacketTypeStunResponse) {
        PacketType mismatchType = packetTypeForPacket(packet);
        int numPacketTypeBytes = numBytesArithmeticCodingFromBuffer(packet.data());
        
        static QMultiMap<QUuid, PacketType> versionDebugSuppressMap;
        
        QUuid senderUUID = uuidFromPacketHeader(packet);
        if (!versionDebugSuppressMap.contains(senderUUID, checkType)) {
            qDebug() << "Packet version mismatch on" << packetTypeForPacket(packet) << "- Sender"
            << uuidFromPacketHeader(packet) << "sent" << qPrintable(QString::number(packet[numPacketTypeBytes])) << "but"
            << qPrintable(QString::number(versionForPacketType(mismatchType))) << "expected.";
            
            versionDebugSuppressMap.insert(senderUUID, checkType);
        }
        
        return false;
    }
    
    const QSet<PacketType> NON_VERIFIED_PACKETS = QSet<PacketType>()
        << PacketTypeDomainServerAuthRequest << PacketTypeDomainConnectRequest
        << PacketTypeStunResponse << PacketTypeDataServerConfirm
        << PacketTypeDataServerGet << PacketTypeDataServerPut << PacketTypeDataServerSend
        << PacketTypeCreateAssignment << PacketTypeRequestAssignment;
    
    if (!NON_VERIFIED_PACKETS.contains(checkType)) {
        // figure out which node this is from
        SharedNodePointer sendingNode = sendingNodeForPacket(packet);
        if (sendingNode) {
            // check if the md5 hash in the header matches the hash we would expect
            if (hashFromPacketHeader(packet) == hashForPacketAndConnectionUUID(packet, sendingNode->getConnectionSecret())) {
                return true;
            } else {
                qDebug() << "Packet hash mismatch on" << checkType << "- Sender"
                    << uuidFromPacketHeader(packet);
            }
        } else {
            if (checkType == PacketTypeDomainList) {
                
                if (_domainInfo.getRootAuthenticationURL().isEmpty() && _domainInfo.getUUID().isNull()) {
                    // if this is a domain-server that doesn't require auth,
                    // pull the UUID from this packet and set it as our domain-server UUID
                    _domainInfo.setUUID(uuidFromPacketHeader(packet));
                    
                    // we also know this domain-server requires no authentication
                    // so set the account manager root URL to the default one
                    AccountManager::getInstance().setAuthURL(DEFAULT_NODE_AUTH_URL);
                }
                
                if (_domainInfo.getUUID() == uuidFromPacketHeader(packet)) {
                    if (hashForPacketAndConnectionUUID(packet, _domainInfo.getConnectionSecret()) == hashFromPacketHeader(packet)) {
                        // this is a packet from the domain-server (PacketTypeDomainServerListRequest)
                        // and the sender UUID matches the UUID we expect for the domain
                        return true;
                    } else {
                        // this is a packet from the domain-server but there is a hash mismatch
                        qDebug() << "Packet hash mismatch on" << checkType << "from domain-server at" << _domainInfo.getHostname();
                        return false;
                    }
                }
            }
            
            qDebug() << "Packet of type" << checkType << "received from unknown node with UUID"
                << uuidFromPacketHeader(packet);
        }
    } else {
        return true;
    }
    
    return false;
}

qint64 NodeList::writeDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr,
                               const QUuid& connectionSecret) {
    QByteArray datagramCopy = datagram;
    
    // setup the MD5 hash for source verification in the header
    replaceHashInPacketGivenConnectionUUID(datagramCopy, connectionSecret);
    
    // stat collection for packets
    ++_numCollectedPackets;
    _numCollectedBytes += datagram.size();
    
    return _nodeSocket.writeDatagram(datagramCopy, destinationSockAddr.getAddress(), destinationSockAddr.getPort());
    
}

qint64 NodeList::writeDatagram(const QByteArray& datagram, const SharedNodePointer& destinationNode,
                               const HifiSockAddr& overridenSockAddr) {
    if (destinationNode) {
        // if we don't have an ovveriden address, assume they want to send to the node's active socket
        const HifiSockAddr* destinationSockAddr = &overridenSockAddr;
        if (overridenSockAddr.isNull()) {
            if (destinationNode->getActiveSocket()) {
                // use the node's active socket as the destination socket
                destinationSockAddr = destinationNode->getActiveSocket();
            } else {
                // we don't have a socket to send to, return 0
                return 0;
            }
        }
        
        writeDatagram(datagram, *destinationSockAddr, destinationNode->getConnectionSecret());
    }
    
    // didn't have a destinationNode to send to, return 0
    return 0;
}

qint64 NodeList::writeDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                               const HifiSockAddr& overridenSockAddr) {
    return writeDatagram(QByteArray(data, size), destinationNode, overridenSockAddr);
}

qint64 NodeList::sendStatsToDomainServer(const QJsonObject& statsObject) {
    QByteArray statsPacket = byteArrayWithPopulatedHeader(PacketTypeNodeJsonStats);
    QDataStream statsPacketStream(&statsPacket, QIODevice::Append);
    
    statsPacketStream << statsObject.toVariantMap();
    
    return writeDatagram(statsPacket, _domainInfo.getSockAddr(), _domainInfo.getConnectionSecret());
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
    sendingNode->setClockSkewUsec(clockSkew);
    
    const bool wantDebug = false;
    
    if (wantDebug) {
        qDebug() << "PING_REPLY from node " << *sendingNode << "\n" <<
        "                     now: " << now << "\n" <<
        "                 ourTime: " << ourOriginalTime << "\n" <<
        "                pingTime: " << pingTime << "\n" <<
        "        oneWayFlightTime: " << oneWayFlightTime << "\n" <<
        "         othersReplyTime: " << othersReplyTime << "\n" <<
        "    othersExprectedReply: " << othersExprectedReply << "\n" <<
        "               clockSkew: " << clockSkew;
    }
}

void NodeList::processNodeData(const HifiSockAddr& senderSockAddr, const QByteArray& packet) {
    switch (packetTypeForPacket(packet)) {
        case PacketTypeDomainList: {
            processDomainServerList(packet);
            break;
        }
        case PacketTypeDomainServerAuthRequest: {
            // the domain-server has asked us to auth via a data-server
            processDomainServerAuthRequest(packet);
            
            break;
        }
        case PacketTypePing: {
            // send back a reply
            SharedNodePointer matchingNode = sendingNodeForPacket(packet);
            if (matchingNode) {
                matchingNode->setLastHeardMicrostamp(usecTimestampNow());
                QByteArray replyPacket = constructPingReplyPacket(packet);
                writeDatagram(replyPacket, matchingNode, senderSockAddr);
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
            // the node decided not to do anything with this packet
            // if it comes from a known source we should keep that node alive
            SharedNodePointer matchingNode = sendingNodeForPacket(packet);
            if (matchingNode) {
                matchingNode->setLastHeardMicrostamp(usecTimestampNow());
            }
            
            break;
    }
}

int NodeList::updateNodeWithDataFromPacket(const SharedNodePointer& matchingNode, const QByteArray &packet) {
    QMutexLocker locker(&matchingNode->getMutex());
    
    matchingNode->setLastHeardMicrostamp(usecTimestampNow());
    matchingNode->recordBytesReceived(packet.size());
    
    if (!matchingNode->getLinkedData() && linkedDataCreateCallback) {
        linkedDataCreateCallback(matchingNode.data());
    }
    
    return matchingNode->getLinkedData()->parseData(packet);
}

int NodeList::findNodeAndUpdateWithDataFromPacket(const QByteArray& packet) {
    SharedNodePointer matchingNode = sendingNodeForPacket(packet);
    
    if (matchingNode) {
        updateNodeWithDataFromPacket(matchingNode, packet);
    }
    
    // we weren't able to match the sender address to the address we have for this node, unlock and don't parse
    return 0;
}

SharedNodePointer NodeList::nodeWithUUID(const QUuid& nodeUUID, bool blockingLock) {
    SharedNodePointer node;
    // if caller wants us to block and guarantee the correct answer, then honor that request
    if (blockingLock) {
        // this will block till we can get access
        QMutexLocker locker(&_nodeHashMutex);
        node = _nodeHash.value(nodeUUID);
    } else if (_nodeHashMutex.tryLock()) { // some callers are willing to get wrong answers but not block
        node = _nodeHash.value(nodeUUID);
        _nodeHashMutex.unlock();
    }
    return node;
}

SharedNodePointer NodeList::sendingNodeForPacket(const QByteArray& packet) {
    QUuid nodeUUID = uuidFromPacketHeader(packet);
    
    // return the matching node, or NULL if there is no match
    return nodeWithUUID(nodeUUID);
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

    // refresh the owner UUID to the NULL UUID
    setSessionUUID(QUuid());
    
    // clear the domain connection information
    _domainInfo.clearConnectionInfo();
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
    if (_publicSockAddr.isNull() && !_hasCompletedInitialSTUNFailure) {
        // we don't know our public socket and we need to send it to the domain server
        // send a STUN request to figure it out
        sendSTUNRequest();
    } else if (!_domainInfo.getIP().isNull()) {
        if (_domainInfo.getRootAuthenticationURL().isEmpty()
            || !_sessionUUID.isNull()
            || !_domainInfo.getRegistrationToken().isEmpty() ) {
            // construct the DS check in packet
            
            PacketType domainPacketType = _sessionUUID.isNull() ? PacketTypeDomainConnectRequest : PacketTypeDomainListRequest;
            
            QUuid packetUUID = (domainPacketType == PacketTypeDomainListRequest)
                ? _sessionUUID : _domainInfo.getAssignmentUUID();
            
            QByteArray domainServerPacket = byteArrayWithPopulatedHeader(domainPacketType, packetUUID);
            QDataStream packetStream(&domainServerPacket, QIODevice::Append);
            
            if (domainPacketType == PacketTypeDomainConnectRequest) {
                // we may need a registration token to present to the domain-server
                packetStream << (quint8) !_domainInfo.getRegistrationToken().isEmpty();
                
                if (!_domainInfo.getRegistrationToken().isEmpty()) {
                    // if we have a registration token send that along in the request
                    packetStream << _domainInfo.getRegistrationToken();
                }
            }
            
            // pack our data to send to the domain-server
            packetStream << _ownerType << _publicSockAddr
                << HifiSockAddr(QHostAddress(getHostOrderLocalAddress()), _nodeSocket.localPort())
                << (quint8) _nodeTypesOfInterest.size();
            
            // copy over the bytes for node types of interest, if required
            foreach (NodeType_t nodeTypeOfInterest, _nodeTypesOfInterest) {
                packetStream << nodeTypeOfInterest;
            }
            
            qDebug() << "sending DS check in size" << domainServerPacket.size() << "to" << _domainInfo.getSockAddr();
            qint64 code = writeDatagram(domainServerPacket, _domainInfo.getSockAddr(), _domainInfo.getConnectionSecret());
            qDebug() << "Code returned is" << code;
            if (code == -1) {
                qDebug() << "the socket error is" << _nodeSocket.errorString();
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
        }  else if (AccountManager::getInstance().hasValidAccessToken()) {
            // we have an access token we can use for the authentication server the domain-server requested
            // so ask that server to provide us with information to connect to the domain-server
            requestAuthForDomainServer();
        }
    }
}

void NodeList::setSessionUUID(const QUuid& sessionUUID) {
    QUuid oldUUID = _sessionUUID;
    _sessionUUID = sessionUUID;
    
    if (sessionUUID != oldUUID) {
        qDebug() << "NodeList UUID changed from" <<  uuidStringWithoutCurlyBraces(oldUUID)
            << "to" << uuidStringWithoutCurlyBraces(_sessionUUID);
        emit uuidChanged(sessionUUID);
    }
}

int NodeList::processDomainServerList(const QByteArray& packet) {
    // this is a packet from the domain server, reset the count of un-replied check-ins
    _numNoReplyDomainCheckIns = 0;
    
    // if this was the first domain-server list from this domain, we've now connected
    _domainInfo.setIsConnected(true);

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
            nodePublicSocket.setAddress(_domainInfo.getIP());
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

void NodeList::domainServerAuthReply(const QJsonObject& jsonObject) {
    _domainInfo.parseAuthInformationFromJsonObject(jsonObject);
}

void NodeList::requestAuthForDomainServer() {
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "domainServerAuthReply";
    
    AccountManager::getInstance().authenticatedRequest("/api/v1/domains/"
                                                       + uuidStringWithoutCurlyBraces(_domainInfo.getUUID()) + "/auth.json",
                                                       QNetworkAccessManager::GetOperation,
                                                       callbackParams);
}

void NodeList::processDomainServerAuthRequest(const QByteArray& packet) {
    QDataStream authPacketStream(packet);
    authPacketStream.skipRawData(numBytesForPacketHeader(packet));
    
    _domainInfo.setUUID(uuidFromPacketHeader(packet));
    AccountManager& accountManager = AccountManager::getInstance();

    // grab the hostname this domain-server wants us to authenticate with
    QUrl authenticationRootURL;
    authPacketStream >> authenticationRootURL;
    
    accountManager.setAuthURL(authenticationRootURL);
    _domainInfo.setRootAuthenticationURL(authenticationRootURL);
    
    if (AccountManager::getInstance().checkAndSignalForAccessToken()) {
        // request a domain-server auth
        requestAuthForDomainServer();
    }
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

void NodeList::pingPublicAndLocalSocketsForInactiveNode(const SharedNodePointer& node) {
    
    // send the ping packet to the local and public sockets for this node
    QByteArray localPingPacket = constructPingPacket(PingType::Local);
    writeDatagram(localPingPacket, node, node->getLocalSocket());
    
    QByteArray publicPingPacket = constructPingPacket(PingType::Public);
    writeDatagram(publicPingPacket, node, node->getPublicSocket());
}

SharedNodePointer NodeList::addOrUpdateNode(const QUuid& uuid, char nodeType,
                                            const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket) {
    _nodeHashMutex.lock();
    
    if (!_nodeHash.contains(uuid)) {
        
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
        
        return updateSocketsForNode(uuid, publicSocket, localSocket);
    }
}

SharedNodePointer NodeList::updateSocketsForNode(const QUuid& uuid,
                                                 const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket) {

    SharedNodePointer matchingNode = nodeWithUUID(uuid);
    
    if (matchingNode) {
        // perform appropriate updates to this node
        QMutexLocker locker(&matchingNode->getMutex());
        
        // check if we need to change this node's public or local sockets
        if (publicSocket != matchingNode->getPublicSocket()) {
            matchingNode->setPublicSocket(publicSocket);
            qDebug() << "Public socket change for node" << *matchingNode;
        }
        
        if (localSocket != matchingNode->getLocalSocket()) {
            matchingNode->setLocalSocket(localSocket);
            qDebug() << "Local socket change for node" << *matchingNode;
        }
    }
    
    return matchingNode;
}

unsigned NodeList::broadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes) {
    unsigned n = 0;

    foreach (const SharedNodePointer& node, getNodeHash()) {
        // only send to the NodeTypes we are asked to send to.
        if (destinationNodeTypes.contains(node->getType())) {
            writeDatagram(packet, node);
            ++n;
        }
    }

    return n;
}

void NodeList::pingInactiveNodes() {
    foreach (const SharedNodePointer& node, getNodeHash()) {
        if (!node->getActiveSocket()) {
            // we don't have an active link to this node, ping it to set that up
            pingPublicAndLocalSocketsForInactiveNode(node);
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
    }
}

SharedNodePointer NodeList::soloNodeOfType(char nodeType) {

    if (memchr(SOLO_NODE_TYPES, nodeType, sizeof(SOLO_NODE_TYPES))) {
        foreach (const SharedNodePointer& node, getNodeHash()) {
            if (node->getType() == nodeType) {
                return node;
            }
        }
    }
    return SharedNodePointer();
}

void NodeList::getPacketStats(float& packetsPerSecond, float& bytesPerSecond) {
    packetsPerSecond = (float) _numCollectedPackets / ((float) _packetStatTimer.elapsed() / 1000.0f);
    bytesPerSecond = (float) _numCollectedBytes / ((float) _packetStatTimer.elapsed() / 1000.0f);
}

void NodeList::resetPacketStats() {
    _numCollectedPackets = 0;
    _numCollectedBytes = 0;
    _packetStatTimer.restart();
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
        _domainInfo.setHostname(domainServerHostname);
    } else {
        _domainInfo.setHostname(DEFAULT_DOMAIN_HOSTNAME);
    }
    
    settings->endGroup();
}

void NodeList::saveData(QSettings* settings) {
    settings->beginGroup(DOMAIN_SERVER_SETTING_KEY);

    if (_domainInfo.getHostname() != DEFAULT_DOMAIN_HOSTNAME) {
        // the user is using a different hostname, store it
        settings->setValue(DOMAIN_SERVER_SETTING_KEY, QVariant(_domainInfo.getHostname()));
    } else {
        // the user has switched back to default, remove the current setting
        settings->remove(DOMAIN_SERVER_SETTING_KEY);
    }

    settings->endGroup();
}
