//
//  LimitedNodeList.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LimitedNodeList.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QUrl>
#include <QtNetwork/QHostInfo>

#include <tbb/parallel_for.h>

#include <LogHandler.h>
#include <NumericalConstants.h>
#include <SharedUtil.h>

#include "AccountManager.h"
#include "Assignment.h"
#include "HifiSockAddr.h"
#include "UUID.h"
#include "NetworkLogging.h"
#include "udt/Packet.h"

const char SOLO_NODE_TYPES[2] = {
    NodeType::AvatarMixer,
    NodeType::AudioMixer
};

LimitedNodeList::LimitedNodeList(unsigned short socketListenPort, unsigned short dtlsListenPort) :
    linkedDataCreateCallback(NULL),
    _sessionUUID(),
    _nodeHash(),
    _nodeMutex(QReadWriteLock::Recursive),
    _nodeSocket(this),
    _dtlsSocket(NULL),
    _localSockAddr(),
    _publicSockAddr(),
    _stunSockAddr(STUN_SERVER_HOSTNAME, STUN_SERVER_PORT),
    _packetReceiver(new PacketReceiver(this)),
    _numCollectedPackets(0),
    _numCollectedBytes(0),
    _packetStatTimer(),
    _thisNodeCanAdjustLocks(false),
    _thisNodeCanRez(true)
{
    static bool firstCall = true;
    if (firstCall) {
        NodeType::init();

        // register the SharedNodePointer meta-type for signals/slots
        qRegisterMetaType<QSharedPointer<Node>>();
        qRegisterMetaType<SharedNodePointer>();

        firstCall = false;
    }
    
    qRegisterMetaType<ConnectionStep>("ConnectionStep");

    _nodeSocket.bind(QHostAddress::AnyIPv4, socketListenPort);
    qCDebug(networking) << "NodeList socket is listening on" << _nodeSocket.localPort();

    if (dtlsListenPort > 0) {
        // only create the DTLS socket during constructor if a custom port is passed
        _dtlsSocket = new QUdpSocket(this);

        _dtlsSocket->bind(QHostAddress::AnyIPv4, dtlsListenPort);
        qCDebug(networking) << "NodeList DTLS socket is listening on" << _dtlsSocket->localPort();
    }

    // check for local socket updates every so often
    const int LOCAL_SOCKET_UPDATE_INTERVAL_MSECS = 5 * 1000;
    QTimer* localSocketUpdate = new QTimer(this);
    connect(localSocketUpdate, &QTimer::timeout, this, &LimitedNodeList::updateLocalSockAddr);
    localSocketUpdate->start(LOCAL_SOCKET_UPDATE_INTERVAL_MSECS);

    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, &QTimer::timeout, this, &LimitedNodeList::removeSilentNodes);
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_MSECS);

    // check the local socket right now
    updateLocalSockAddr();
    
    // set &PacketReceiver::handleVerifiedPacket as the verified packet callback for the udt::Socket
    using std::placeholders::_1;
    _nodeSocket.setPacketHandler(std::bind(&PacketReceiver::handleVerifiedPacket, _packetReceiver, _1));
    _nodeSocket.setPacketListHandler(std::bind(&PacketReceiver::handleVerifiedPacketList, _packetReceiver, _1));

    // set our isPacketVerified method as the verify operator for the udt::Socket
    _nodeSocket.setPacketFilterOperator(std::bind(&LimitedNodeList::isPacketVerified, this, _1));
    
    _packetStatTimer.start();
}

void LimitedNodeList::setSessionUUID(const QUuid& sessionUUID) {
    QUuid oldUUID = _sessionUUID;
    _sessionUUID = sessionUUID;

    if (sessionUUID != oldUUID) {
        qCDebug(networking) << "NodeList UUID changed from" <<  uuidStringWithoutCurlyBraces(oldUUID)
        << "to" << uuidStringWithoutCurlyBraces(_sessionUUID);
        emit uuidChanged(sessionUUID, oldUUID);
    }
}

void LimitedNodeList::setThisNodeCanAdjustLocks(bool canAdjustLocks) {
    if (_thisNodeCanAdjustLocks != canAdjustLocks) {
        _thisNodeCanAdjustLocks = canAdjustLocks;
        emit canAdjustLocksChanged(canAdjustLocks);
    }
}

void LimitedNodeList::setThisNodeCanRez(bool canRez) {
    if (_thisNodeCanRez != canRez) {
        _thisNodeCanRez = canRez;
        emit canRezChanged(canRez);
    }
}

QUdpSocket& LimitedNodeList::getDTLSSocket() {
    if (!_dtlsSocket) {
        // DTLS socket getter called but no DTLS socket exists, create it now
        _dtlsSocket = new QUdpSocket(this);

        _dtlsSocket->bind(QHostAddress::AnyIPv4, 0, QAbstractSocket::DontShareAddress);

        // we're using DTLS and our socket is good to go, so make the required DTLS changes
        // DTLS requires that IP_DONTFRAG be set
        // This is not accessible on some platforms (OS X) so we need to make sure DTLS still works without it

        qCDebug(networking) << "LimitedNodeList DTLS socket is listening on" << _dtlsSocket->localPort();
    }

    return *_dtlsSocket;
}

bool LimitedNodeList::isPacketVerified(const udt::Packet& packet) {
    return packetVersionMatch(packet) && packetSourceAndHashMatch(packet);
}

bool LimitedNodeList::packetVersionMatch(const udt::Packet& packet) {
    PacketType headerType = NLPacket::typeInHeader(packet);
    PacketVersion headerVersion = NLPacket::versionInHeader(packet);
    
    if (headerVersion != versionForPacketType(headerType)) {
        
        static QMultiHash<QUuid, PacketType> sourcedVersionDebugSuppressMap;
        static QMultiHash<HifiSockAddr, PacketType> versionDebugSuppressMap;
        
        bool hasBeenOutput = false;
        QString senderString;
        
        if (NON_SOURCED_PACKETS.contains(headerType)) {
            const HifiSockAddr& senderSockAddr = packet.getSenderSockAddr();
            hasBeenOutput = versionDebugSuppressMap.contains(senderSockAddr, headerType);
            
            if (!hasBeenOutput) {
                versionDebugSuppressMap.insert(senderSockAddr, headerType);
                senderString = QString("%1:%2").arg(senderSockAddr.getAddress().toString()).arg(senderSockAddr.getPort());
            }
        } else {
            QUuid sourceID = NLPacket::sourceIDInHeader(packet);
            
            hasBeenOutput = sourcedVersionDebugSuppressMap.contains(sourceID, headerType);
            
            if (!hasBeenOutput) {
                sourcedVersionDebugSuppressMap.insert(sourceID, headerType);
                senderString = uuidStringWithoutCurlyBraces(sourceID.toString());
            }
        }
        
        if (!hasBeenOutput) {
            qCDebug(networking) << "Packet version mismatch on" << headerType << "- Sender"
                << senderString << "sent" << qPrintable(QString::number(headerVersion)) << "but"
                << qPrintable(QString::number(versionForPacketType(headerType))) << "expected.";
            
            emit packetVersionMismatch(headerType);
        }
        
        return false;
    } else {
        return true;
    }
}

bool LimitedNodeList::packetSourceAndHashMatch(const udt::Packet& packet) {
    
    PacketType headerType = NLPacket::typeInHeader(packet);
    
    if (NON_SOURCED_PACKETS.contains(headerType)) {
        return true;
    } else {
        QUuid sourceID = NLPacket::sourceIDInHeader(packet);
        
        // figure out which node this is from
        SharedNodePointer matchingNode = nodeWithUUID(sourceID);
        
        if (matchingNode) {
            if (!NON_VERIFIED_PACKETS.contains(headerType)) {
                
                QByteArray packetHeaderHash = NLPacket::verificationHashInHeader(packet);
                QByteArray expectedHash = NLPacket::hashForPacketAndSecret(packet, matchingNode->getConnectionSecret());
            
                // check if the md5 hash in the header matches the hash we would expect
                if (packetHeaderHash != expectedHash) {
                    static QMultiMap<QUuid, PacketType> hashDebugSuppressMap;
                    
                    if (!hashDebugSuppressMap.contains(sourceID, headerType)) {
                        qCDebug(networking) << "Packet hash mismatch on" << headerType << "- Sender" << sourceID;

                        hashDebugSuppressMap.insert(sourceID, headerType);
                    }

                    return false;
                }
            }
            
            return true;
            
        } else {
            static QString repeatedMessage
                = LogHandler::getInstance().addRepeatedMessageRegex("Packet of type \\d+ \\([\\sa-zA-Z]+\\) received from unknown node with UUID");

            qCDebug(networking) << "Packet of type" << headerType 
                << "received from unknown node with UUID" << qPrintable(uuidStringWithoutCurlyBraces(sourceID));
        }
    }

    return false;
}

void LimitedNodeList::collectPacketStats(const NLPacket& packet) {
    // stat collection for packets
    ++_numCollectedPackets;
    _numCollectedBytes += packet.getDataSize();
}

void LimitedNodeList::fillPacketHeader(const NLPacket& packet, const QUuid& connectionSecret) {
    if (!NON_SOURCED_PACKETS.contains(packet.getType())) {
        packet.writeSourceID(getSessionUUID());
    }
    
    if (!connectionSecret.isNull()
        && !NON_SOURCED_PACKETS.contains(packet.getType())
        && !NON_VERIFIED_PACKETS.contains(packet.getType())) {
        packet.writeVerificationHashGivenSecret(connectionSecret);
    }
}

qint64 LimitedNodeList::sendUnreliablePacket(const NLPacket& packet, const Node& destinationNode) {
    Q_ASSERT(!packet.isPartOfMessage());
    if (!destinationNode.getActiveSocket()) {
        return 0;
    }
    emit dataSent(destinationNode.getType(), packet.getDataSize());
    return sendUnreliablePacket(packet, *destinationNode.getActiveSocket(), destinationNode.getConnectionSecret());
}

qint64 LimitedNodeList::sendUnreliablePacket(const NLPacket& packet, const HifiSockAddr& sockAddr,
                                             const QUuid& connectionSecret) {
    Q_ASSERT(!packet.isPartOfMessage());
    Q_ASSERT_X(!packet.isReliable(), "LimitedNodeList::sendUnreliablePacket",
               "Trying to send a reliable packet unreliably.");
    
    collectPacketStats(packet);
    fillPacketHeader(packet, connectionSecret);
    
    return _nodeSocket.writePacket(packet, sockAddr);
}

qint64 LimitedNodeList::sendPacket(std::unique_ptr<NLPacket> packet, const Node& destinationNode) {
    Q_ASSERT(!packet->isPartOfMessage());
    if (!destinationNode.getActiveSocket()) {
        return 0;
    }
    emit dataSent(destinationNode.getType(), packet->getDataSize());
    return sendPacket(std::move(packet), *destinationNode.getActiveSocket(), destinationNode.getConnectionSecret());
}

qint64 LimitedNodeList::sendPacket(std::unique_ptr<NLPacket> packet, const HifiSockAddr& sockAddr,
                                   const QUuid& connectionSecret) {
    Q_ASSERT(!packet->isPartOfMessage());
    if (packet->isReliable()) {
        collectPacketStats(*packet);
        fillPacketHeader(*packet, connectionSecret);
        
        auto size = packet->getDataSize();
        _nodeSocket.writePacket(std::move(packet), sockAddr);
        
        return size;
    } else {
        return sendUnreliablePacket(*packet, sockAddr, connectionSecret);
    }
}

qint64 LimitedNodeList::sendPacketList(NLPacketList& packetList, const Node& destinationNode) {
    auto activeSocket = destinationNode.getActiveSocket();
    if (!activeSocket) {
        return 0;
    }
    qint64 bytesSent = 0;
    auto connectionSecret = destinationNode.getConnectionSecret();
    
    // close the last packet in the list
    packetList.closeCurrentPacket();
    
    while (!packetList._packets.empty()) {
        bytesSent += sendPacket(packetList.takeFront<NLPacket>(), *activeSocket, connectionSecret);
    }
    
    emit dataSent(destinationNode.getType(), bytesSent);
    return bytesSent;
}

qint64 LimitedNodeList::sendPacketList(NLPacketList& packetList, const HifiSockAddr& sockAddr,
                                       const QUuid& connectionSecret) {
    qint64 bytesSent = 0;
    
    // close the last packet in the list
    packetList.closeCurrentPacket();
    
    while (!packetList._packets.empty()) {
        bytesSent += sendPacket(packetList.takeFront<NLPacket>(), sockAddr, connectionSecret);
    }
    
    return bytesSent;
}

qint64 LimitedNodeList::sendPacketList(std::unique_ptr<NLPacketList> packetList, const HifiSockAddr& sockAddr) {
    // close the last packet in the list
    packetList->closeCurrentPacket();
    
    return _nodeSocket.writePacketList(std::move(packetList), sockAddr);
}

qint64 LimitedNodeList::sendPacketList(std::unique_ptr<NLPacketList> packetList, const Node& destinationNode) {
    // close the last packet in the list
    packetList->closeCurrentPacket();

    for (std::unique_ptr<udt::Packet>& packet : packetList->_packets) {
        NLPacket* nlPacket = static_cast<NLPacket*>(packet.get());
        collectPacketStats(*nlPacket);
        fillPacketHeader(*nlPacket, destinationNode.getConnectionSecret());
    }

    return _nodeSocket.writePacketList(std::move(packetList), *destinationNode.getActiveSocket());
}

qint64 LimitedNodeList::sendPacket(std::unique_ptr<NLPacket> packet, const Node& destinationNode,
                                   const HifiSockAddr& overridenSockAddr) {
    // use the node's active socket as the destination socket if there is no overriden socket address
    auto& destinationSockAddr = (overridenSockAddr.isNull()) ? *destinationNode.getActiveSocket()
                                                             : overridenSockAddr;
    return sendPacket(std::move(packet), destinationSockAddr, destinationNode.getConnectionSecret());
}

int LimitedNodeList::updateNodeWithDataFromPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {
    QMutexLocker locker(&sendingNode->getMutex());

    NodeData* linkedData = sendingNode->getLinkedData();
    if (!linkedData && linkedDataCreateCallback) {
        linkedDataCreateCallback(sendingNode.data());
    }

    if (linkedData) {
        QMutexLocker linkedDataLocker(&linkedData->getMutex());
        return linkedData->parseData(*packet);
    }
    
    return 0;
}

SharedNodePointer LimitedNodeList::nodeWithUUID(const QUuid& nodeUUID) {
    QReadLocker readLocker(&_nodeMutex);

    NodeHash::const_iterator it = _nodeHash.find(nodeUUID);
    return it == _nodeHash.cend() ? SharedNodePointer() : it->second;
 }

void LimitedNodeList::eraseAllNodes() {
    qCDebug(networking) << "Clearing the NodeList. Deleting all nodes in list.";

    QSet<SharedNodePointer> killedNodes;
    eachNode([&killedNodes](const SharedNodePointer& node){
        killedNodes.insert(node);
    });

    // iterate the current nodes, emit that they are dying and remove them from the hash
    _nodeMutex.lockForWrite();
    _nodeHash.clear();
    _nodeMutex.unlock();

    foreach(const SharedNodePointer& killedNode, killedNodes) {
        handleNodeKill(killedNode);
    }
}

void LimitedNodeList::reset() {
    eraseAllNodes();
    
    // we need to make sure any socket connections are gone so wait on that here
    _nodeSocket.clearConnections();
}

void LimitedNodeList::killNodeWithUUID(const QUuid& nodeUUID) {
    _nodeMutex.lockForRead();

    NodeHash::iterator it = _nodeHash.find(nodeUUID);
    if (it != _nodeHash.end()) {
        SharedNodePointer matchingNode = it->second;

        _nodeMutex.unlock();

        _nodeMutex.lockForWrite();
        _nodeHash.unsafe_erase(it);
        _nodeMutex.unlock();

        handleNodeKill(matchingNode);
    } else {
        _nodeMutex.unlock();
    }
}

void LimitedNodeList::processKillNode(NLPacket& packet) {
    // read the node id
    QUuid nodeUUID = QUuid::fromRfc4122(packet.readWithoutCopy(NUM_BYTES_RFC4122_UUID));

    // kill the node with this UUID, if it exists
    killNodeWithUUID(nodeUUID);
}

void LimitedNodeList::handleNodeKill(const SharedNodePointer& node) {
    qCDebug(networking) << "Killed" << *node;
    node->stopPingTimer();
    emit nodeKilled(node);
    
    if (auto activeSocket = node->getActiveSocket()) {
        _nodeSocket.cleanupConnection(*activeSocket);
    }
}

SharedNodePointer LimitedNodeList::addOrUpdateNode(const QUuid& uuid, NodeType_t nodeType,
                                                   const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket,
                                                   bool canAdjustLocks, bool canRez,
                                                   const QUuid& connectionSecret) {
    NodeHash::const_iterator it = _nodeHash.find(uuid);

    if (it != _nodeHash.end()) {
        SharedNodePointer& matchingNode = it->second;

        matchingNode->setPublicSocket(publicSocket);
        matchingNode->setLocalSocket(localSocket);
        matchingNode->setCanAdjustLocks(canAdjustLocks);
        matchingNode->setCanRez(canRez);
        matchingNode->setConnectionSecret(connectionSecret);

        return matchingNode;
    } else {
        // we didn't have this node, so add them
        Node* newNode = new Node(uuid, nodeType, publicSocket, localSocket, canAdjustLocks, canRez, connectionSecret, this);

        if (nodeType == NodeType::AudioMixer) {
            LimitedNodeList::flagTimeForConnectionStep(LimitedNodeList::AddedAudioMixer);
        }

        SharedNodePointer newNodePointer(newNode);

        _nodeHash.insert(UUIDNodePair(newNode->getUUID(), newNodePointer));

        qCDebug(networking) << "Added" << *newNode;

        emit nodeAdded(newNodePointer);

        return newNodePointer;
    }
}

std::unique_ptr<NLPacket> LimitedNodeList::constructPingPacket(PingType_t pingType) {
    int packetSize = sizeof(PingType_t) + sizeof(quint64);
    
    auto pingPacket = NLPacket::create(PacketType::Ping, packetSize);
    
    pingPacket->writePrimitive(pingType);
    pingPacket->writePrimitive(usecTimestampNow());

    return std::move(pingPacket);
}

std::unique_ptr<NLPacket> LimitedNodeList::constructPingReplyPacket(NLPacket& pingPacket) {
    PingType_t typeFromOriginalPing;
    quint64 timeFromOriginalPing;
    pingPacket.readPrimitive(&typeFromOriginalPing);
    pingPacket.readPrimitive(&timeFromOriginalPing);

    int packetSize = sizeof(PingType_t) + sizeof(quint64) + sizeof(quint64);
    auto replyPacket = NLPacket::create(PacketType::PingReply, packetSize);
    replyPacket->writePrimitive(typeFromOriginalPing);
    replyPacket->writePrimitive(timeFromOriginalPing);
    replyPacket->writePrimitive(usecTimestampNow());

    return std::move(replyPacket);
}

std::unique_ptr<NLPacket> LimitedNodeList::constructICEPingPacket(PingType_t pingType, const QUuid& iceID) {
    int packetSize = NUM_BYTES_RFC4122_UUID + sizeof(PingType_t);

    auto icePingPacket = NLPacket::create(PacketType::ICEPing, packetSize);
    icePingPacket->write(iceID.toRfc4122());
    icePingPacket->writePrimitive(pingType);

    return std::move(icePingPacket);
}

std::unique_ptr<NLPacket> LimitedNodeList::constructICEPingReplyPacket(NLPacket& pingPacket, const QUuid& iceID) {
    // pull out the ping type so we can reply back with that
    PingType_t pingType;

    memcpy(&pingType, pingPacket.getPayload() + NUM_BYTES_RFC4122_UUID, sizeof(PingType_t));

    int packetSize = NUM_BYTES_RFC4122_UUID + sizeof(PingType_t);
    auto icePingReplyPacket = NLPacket::create(PacketType::ICEPingReply, packetSize);

    // pack the ICE ID and then the ping type
    icePingReplyPacket->write(iceID.toRfc4122());
    icePingReplyPacket->writePrimitive(pingType);

    return icePingReplyPacket;
}

unsigned int LimitedNodeList::broadcastToNodes(std::unique_ptr<NLPacket> packet, const NodeSet& destinationNodeTypes) {
    unsigned int n = 0;
    
    eachNode([&](const SharedNodePointer& node){
        if (node && destinationNodeTypes.contains(node->getType())) {
            sendUnreliablePacket(*packet, *node);
            ++n;
        }
    });
    
    return n;
}

SharedNodePointer LimitedNodeList::soloNodeOfType(NodeType_t nodeType) {
    return nodeMatchingPredicate([&](const SharedNodePointer& node){
        return node->getType() == nodeType;
    });
}

void LimitedNodeList::getPacketStats(float& packetsPerSecond, float& bytesPerSecond) {
    packetsPerSecond = (float) _numCollectedPackets / ((float) _packetStatTimer.elapsed() / 1000.0f);
    bytesPerSecond = (float) _numCollectedBytes / ((float) _packetStatTimer.elapsed() / 1000.0f);
}

void LimitedNodeList::resetPacketStats() {
    _numCollectedPackets = 0;
    _numCollectedBytes = 0;
    _packetStatTimer.restart();
}

void LimitedNodeList::removeSilentNodes() {

    QSet<SharedNodePointer> killedNodes;

    eachNodeHashIterator([&](NodeHash::iterator& it){
        SharedNodePointer node = it->second;
        node->getMutex().lock();

        if ((usecTimestampNow() - node->getLastHeardMicrostamp()) > (NODE_SILENCE_THRESHOLD_MSECS * USECS_PER_MSEC)) {
            // call the NodeHash erase to get rid of this node
            it = _nodeHash.unsafe_erase(it);

            killedNodes.insert(node);
        } else {
            // we didn't erase this node, push the iterator forwards
            ++it;
        }

        node->getMutex().unlock();
    });

    foreach(const SharedNodePointer& killedNode, killedNodes) {
        handleNodeKill(killedNode);
    }
}

const uint32_t RFC_5389_MAGIC_COOKIE = 0x2112A442;
const int NUM_BYTES_STUN_HEADER = 20;

void LimitedNodeList::sendSTUNRequest() {

    const int NUM_INITIAL_STUN_REQUESTS_BEFORE_FAIL = 10;

    if (!_hasCompletedInitialSTUN) {
        qCDebug(networking) << "Sending intial stun request to" << STUN_SERVER_HOSTNAME;

        if (_numInitialSTUNRequests > NUM_INITIAL_STUN_REQUESTS_BEFORE_FAIL) {
            // we're still trying to do our initial STUN we're over the fail threshold
            stopInitialSTUNUpdate(false);
        }

        ++_numInitialSTUNRequests;
    }

    char stunRequestPacket[NUM_BYTES_STUN_HEADER];

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

    flagTimeForConnectionStep(ConnectionStep::SendSTUNRequest);

    _nodeSocket.writeDatagram(stunRequestPacket, sizeof(stunRequestPacket), _stunSockAddr);
}

void LimitedNodeList::processSTUNResponse(std::unique_ptr<udt::BasePacket> packet) {
    // check the cookie to make sure this is actually a STUN response
    // and read the first attribute and make sure it is a XOR_MAPPED_ADDRESS
    const int NUM_BYTES_MESSAGE_TYPE_AND_LENGTH = 4;
    const uint16_t XOR_MAPPED_ADDRESS_TYPE = htons(0x0020);

    const uint32_t RFC_5389_MAGIC_COOKIE_NETWORK_ORDER = htonl(RFC_5389_MAGIC_COOKIE);

    int attributeStartIndex = NUM_BYTES_STUN_HEADER;

    if (memcmp(packet->getData() + NUM_BYTES_MESSAGE_TYPE_AND_LENGTH,
               &RFC_5389_MAGIC_COOKIE_NETWORK_ORDER,
               sizeof(RFC_5389_MAGIC_COOKIE_NETWORK_ORDER)) == 0) {

        // enumerate the attributes to find XOR_MAPPED_ADDRESS_TYPE
        while (attributeStartIndex < packet->getDataSize()) {
            
            if (memcmp(packet->getData() + attributeStartIndex, &XOR_MAPPED_ADDRESS_TYPE, sizeof(XOR_MAPPED_ADDRESS_TYPE)) == 0) {
                const int NUM_BYTES_STUN_ATTR_TYPE_AND_LENGTH = 4;
                const int NUM_BYTES_FAMILY_ALIGN = 1;
                const uint8_t IPV4_FAMILY_NETWORK_ORDER = htons(0x01) >> 8;

                int byteIndex = attributeStartIndex + NUM_BYTES_STUN_ATTR_TYPE_AND_LENGTH + NUM_BYTES_FAMILY_ALIGN;

                uint8_t addressFamily = 0;
                memcpy(&addressFamily, packet->getData() + byteIndex, sizeof(addressFamily));

                byteIndex += sizeof(addressFamily);

                if (addressFamily == IPV4_FAMILY_NETWORK_ORDER) {
                    // grab the X-Port
                    uint16_t xorMappedPort = 0;
                    memcpy(&xorMappedPort, packet->getData() + byteIndex, sizeof(xorMappedPort));

                    uint16_t newPublicPort = ntohs(xorMappedPort) ^ (ntohl(RFC_5389_MAGIC_COOKIE_NETWORK_ORDER) >> 16);

                    byteIndex += sizeof(xorMappedPort);

                    // grab the X-Address
                    uint32_t xorMappedAddress = 0;
                    memcpy(&xorMappedAddress, packet->getData() + byteIndex, sizeof(xorMappedAddress));

                    uint32_t stunAddress = ntohl(xorMappedAddress) ^ ntohl(RFC_5389_MAGIC_COOKIE_NETWORK_ORDER);

                    QHostAddress newPublicAddress(stunAddress);
                    
                    if (newPublicAddress != _publicSockAddr.getAddress() || newPublicPort != _publicSockAddr.getPort()) {
                        _publicSockAddr = HifiSockAddr(newPublicAddress, newPublicPort);

                        qCDebug(networking, "New public socket received from STUN server is %s:%hu",
                               _publicSockAddr.getAddress().toString().toLocal8Bit().constData(),
                               _publicSockAddr.getPort());

                        if (!_hasCompletedInitialSTUN) {
                            // if we're here we have definitely completed our initial STUN sequence
                            stopInitialSTUNUpdate(true);
                        }

                        emit publicSockAddrChanged(_publicSockAddr);

                        flagTimeForConnectionStep(ConnectionStep::SetPublicSocketFromSTUN);
                    }
                    
                    // we're done reading the packet so we can return now
                    return;
                }
            } else {
                // push forward attributeStartIndex by the length of this attribute
                const int NUM_BYTES_ATTRIBUTE_TYPE = 2;

                uint16_t attributeLength = 0;
                memcpy(&attributeLength, packet->getData() + attributeStartIndex + NUM_BYTES_ATTRIBUTE_TYPE,
                       sizeof(attributeLength));
                attributeLength = ntohs(attributeLength);

                attributeStartIndex += NUM_BYTES_MESSAGE_TYPE_AND_LENGTH + attributeLength;
            }
        }
    }
}

void LimitedNodeList::startSTUNPublicSocketUpdate() {
    assert(!_initialSTUNTimer);

    if (!_initialSTUNTimer) {
        // if we don't know the STUN IP yet we need to have ourselves be called once it is known
        if (_stunSockAddr.getAddress().isNull()) {
            connect(&_stunSockAddr, &HifiSockAddr::lookupCompleted, this, &LimitedNodeList::startSTUNPublicSocketUpdate);
            connect(&_stunSockAddr, &HifiSockAddr::lookupCompleted, this, &LimitedNodeList::addSTUNHandlerToUnfiltered);

            // in case we just completely fail to lookup the stun socket - add a 10s timeout that will trigger the fail case
            const quint64 STUN_DNS_LOOKUP_TIMEOUT_MSECS = 10 * 1000;

            QTimer* stunLookupFailTimer = new QTimer(this);
            connect(stunLookupFailTimer, &QTimer::timeout, this, &LimitedNodeList::possiblyTimeoutSTUNAddressLookup);
            stunLookupFailTimer->start(STUN_DNS_LOOKUP_TIMEOUT_MSECS);

        } else {
            // setup our initial STUN timer here so we can quickly find out our public IP address
            _initialSTUNTimer = new QTimer(this);

            connect(_initialSTUNTimer.data(), &QTimer::timeout, this, &LimitedNodeList::sendSTUNRequest);

            const int STUN_INITIAL_UPDATE_INTERVAL_MSECS = 250;
           _initialSTUNTimer->start(STUN_INITIAL_UPDATE_INTERVAL_MSECS);

           // send an initial STUN request right away
           sendSTUNRequest();
        }
    }
}

void LimitedNodeList::possiblyTimeoutSTUNAddressLookup() {
    if (_stunSockAddr.getAddress().isNull()) {
        // our stun address is still NULL, but we've been waiting for long enough - time to force a fail
        stopInitialSTUNUpdate(false);
    }
}

void LimitedNodeList::addSTUNHandlerToUnfiltered() {
    // make ourselves the handler of STUN packets when they come in
    using std::placeholders::_1;
    _nodeSocket.addUnfilteredHandler(_stunSockAddr, std::bind(&LimitedNodeList::processSTUNResponse, this, _1));
}

void LimitedNodeList::stopInitialSTUNUpdate(bool success) {
    _hasCompletedInitialSTUN = true;

    if (!success) {
        // if we're here this was the last failed STUN request
        // use our DS as our stun server
        qCDebug(networking, "Failed to lookup public address via STUN server at %s:%hu.",
                STUN_SERVER_HOSTNAME, STUN_SERVER_PORT);
        qCDebug(networking) << "LimitedNodeList public socket will be set with local port and null QHostAddress.";

        // reset the public address and port to a null address
        _publicSockAddr = HifiSockAddr(QHostAddress(), _nodeSocket.localPort());

        // we have changed the publicSockAddr, so emit our signal
        emit publicSockAddrChanged(_publicSockAddr);

        flagTimeForConnectionStep(ConnectionStep::SetPublicSocketFromSTUN);
    }

    // stop our initial fast timer
    if (_initialSTUNTimer) {
        _initialSTUNTimer->stop();
        _initialSTUNTimer->deleteLater();
    }

    // We now setup a timer here to fire every so often to check that our IP address has not changed.
    // Or, if we failed - if will check if we can eventually get a public socket
    const int STUN_IP_ADDRESS_CHECK_INTERVAL_MSECS = 30 * 1000;

    QTimer* stunOccasionalTimer = new QTimer(this);
    connect(stunOccasionalTimer, &QTimer::timeout, this, &LimitedNodeList::sendSTUNRequest);

    stunOccasionalTimer->start(STUN_IP_ADDRESS_CHECK_INTERVAL_MSECS);
}

void LimitedNodeList::updateLocalSockAddr() {
    HifiSockAddr newSockAddr(getLocalAddress(), _nodeSocket.localPort());
    if (newSockAddr != _localSockAddr) {

        if (_localSockAddr.isNull()) {
            qCDebug(networking) << "Local socket is" << newSockAddr;
        } else {
            qCDebug(networking) << "Local socket has changed from" << _localSockAddr << "to" << newSockAddr;
        }

        _localSockAddr = newSockAddr;

        emit localSockAddrChanged(_localSockAddr);
    }
}

void LimitedNodeList::sendHeartbeatToIceServer(const HifiSockAddr& iceServerSockAddr) {
    sendPacketToIceServer(PacketType::ICEServerHeartbeat, iceServerSockAddr, _sessionUUID);
}

void LimitedNodeList::sendPeerQueryToIceServer(const HifiSockAddr& iceServerSockAddr, const QUuid& clientID,
                                               const QUuid& peerID) {
    sendPacketToIceServer(PacketType::ICEServerQuery, iceServerSockAddr, clientID, peerID);
}

void LimitedNodeList::sendPacketToIceServer(PacketType packetType, const HifiSockAddr& iceServerSockAddr,
                                            const QUuid& clientID, const QUuid& peerID) {
    auto icePacket = NLPacket::create(packetType);

    QDataStream iceDataStream(icePacket.get());
    iceDataStream << clientID << _publicSockAddr << _localSockAddr;
    
    if (packetType == PacketType::ICEServerQuery) {
        assert(!peerID.isNull());

        iceDataStream << peerID;

        qCDebug(networking) << "Sending packet to ICE server to request connection info for peer with ID"
            << uuidStringWithoutCurlyBraces(peerID);
    }

    sendPacket(std::move(icePacket), iceServerSockAddr);
}

void LimitedNodeList::putLocalPortIntoSharedMemory(const QString key, QObject* parent, quint16 localPort) {
    // save our local port to shared memory so that assignment client children know how to talk to this parent
    QSharedMemory* sharedPortMem = new QSharedMemory(key, parent);

    // attempt to create the shared memory segment
    if (sharedPortMem->create(sizeof(localPort)) || sharedPortMem->attach()) {
        sharedPortMem->lock();
        memcpy(sharedPortMem->data(), &localPort, sizeof(localPort));
        sharedPortMem->unlock();

        qCDebug(networking) << "Wrote local listening port" << localPort << "to shared memory at key" << key;
    } else {
        qWarning() << "Failed to create and attach to shared memory to share local port with assignment-client children.";
    }
}


bool LimitedNodeList::getLocalServerPortFromSharedMemory(const QString key, quint16& localPort) {
    QSharedMemory sharedMem(key);
    if (!sharedMem.attach(QSharedMemory::ReadOnly)) {
        qWarning() << "Could not attach to shared memory at key" << key;
        return false;
    } else {
        sharedMem.lock();
        memcpy(&localPort, sharedMem.data(), sizeof(localPort));
        sharedMem.unlock();
        return true;
    }
}

void LimitedNodeList::flagTimeForConnectionStep(ConnectionStep connectionStep) {
    QMetaObject::invokeMethod(this, "flagTimeForConnectionStep",
                              Q_ARG(ConnectionStep, connectionStep),
                              Q_ARG(quint64, usecTimestampNow()));
}

void LimitedNodeList::flagTimeForConnectionStep(ConnectionStep connectionStep, quint64 timestamp) {

    if (connectionStep == ConnectionStep::LookupAddress) {
        QWriteLocker writeLock(&_connectionTimeLock);

        // we clear the current times if the user just fired off a lookup
        _lastConnectionTimes.clear();
        _areConnectionTimesComplete = false;

        _lastConnectionTimes[timestamp] = connectionStep;
    } else if (!_areConnectionTimesComplete) {
        QWriteLocker writeLock(&_connectionTimeLock);


        // anything > than sending the first DS check should not come before the DS check in, so we drop those
        // this handles the case where you lookup an address and get packets in the existing domain before changing domains
        if (connectionStep > LimitedNodeList::ConnectionStep::SendDSCheckIn
            && (_lastConnectionTimes.key(ConnectionStep::SendDSCheckIn) == 0
                || timestamp <= _lastConnectionTimes.key(ConnectionStep::SendDSCheckIn))) {
            return;
        }

        // if there is no time for existing step add a timestamp on the first call for each ConnectionStep
        _lastConnectionTimes[timestamp] = connectionStep;

        // if this is a received audio packet we consider our connection times complete
        if (connectionStep == ConnectionStep::ReceiveFirstAudioPacket) {
            _areConnectionTimesComplete = true;
        }
    }
}
