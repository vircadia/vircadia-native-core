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

    const int LARGER_BUFFER_SIZE = 1048576;
    changeSocketBufferSizes(LARGER_BUFFER_SIZE);

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

void LimitedNodeList::changeSocketBufferSizes(int numBytes) {
    for (int i = 0; i < 2; i++) {
        QAbstractSocket::SocketOption bufferOpt;
        QString bufferTypeString;
        if (i == 0) {
            bufferOpt = QAbstractSocket::SendBufferSizeSocketOption;
            bufferTypeString = "send";

        } else {
            bufferOpt = QAbstractSocket::ReceiveBufferSizeSocketOption;
            bufferTypeString = "receive";
        }
        int oldBufferSize = _nodeSocket.socketOption(bufferOpt).toInt();
        if (oldBufferSize < numBytes) {
            int newBufferSize = _nodeSocket.socketOption(bufferOpt).toInt();

            qCDebug(networking) << "Changed socket" << bufferTypeString << "buffer size from" << oldBufferSize << "to"
                << newBufferSize << "bytes";
        } else {
            // don't make the buffer smaller
            qCDebug(networking) << "Did not change socket" << bufferTypeString << "buffer size from" << oldBufferSize
                << "since it is larger than desired size of" << numBytes;
        }
    }
}

bool LimitedNodeList::packetVersionAndHashMatch(const QByteArray& packet) {
    PacketType::Value checkType = packetTypeForPacket(packet);
    int numPacketTypeBytes = numBytesArithmeticCodingFromBuffer(packet.data());

    if (packet[numPacketTypeBytes] != versionForPacketType(checkType)
        && checkType != PacketType::StunResponse) {
        PacketType::Value mismatchType = packetTypeForPacket(packet);

        static QMultiMap<QUuid, PacketType::Value> versionDebugSuppressMap;

        QUuid senderUUID = uuidFromPacketHeader(packet);
        if (!versionDebugSuppressMap.contains(senderUUID, checkType)) {
            qCDebug(networking) << "Packet version mismatch on" << packetTypeForPacket(packet) << "- Sender"
            << uuidFromPacketHeader(packet) << "sent" << qPrintable(QString::number(packet[numPacketTypeBytes])) << "but"
            << qPrintable(QString::number(versionForPacketType(mismatchType))) << "expected.";

            emit packetVersionMismatch();

            versionDebugSuppressMap.insert(senderUUID, checkType);
        }

        return false;
    }

    if (!NON_VERIFIED_PACKETS.contains(checkType)) {
        // figure out which node this is from
        SharedNodePointer sendingNode = sendingNodeForPacket(packet);
        if (sendingNode) {
            // check if the md5 hash in the header matches the hash we would expect
            if (hashFromPacketHeader(packet) == hashForPacketAndConnectionUUID(packet, sendingNode->getConnectionSecret())) {
                return true;
            } else {
                static QMultiMap<QUuid, PacketType::Value> hashDebugSuppressMap;

                QUuid senderUUID = uuidFromPacketHeader(packet);
                if (!hashDebugSuppressMap.contains(senderUUID, checkType)) {
                    qCDebug(networking) << "Packet hash mismatch on" << checkType << "- Sender"
                    << uuidFromPacketHeader(packet);

                    hashDebugSuppressMap.insert(senderUUID, checkType);
                }
            }
        } else {
            static QString repeatedMessage
                = LogHandler::getInstance().addRepeatedMessageRegex("Packet of type \\d+ received from unknown node with UUID");

            qCDebug(networking) << "Packet of type" << checkType << "received from unknown node with UUID"
                << qPrintable(uuidStringWithoutCurlyBraces(uuidFromPacketHeader(packet)));
        }
    } else {
        return true;
    }

    return false;
}

// qint64 LimitedNodeList::readDatagram(char* data, qint64 maxSize, QHostAddress* address = 0, quint16 * port = 0) {

qint64 LimitedNodeList::readDatagram(QByteArray& incomingPacket, QHostAddress* address = 0, quint16 * port = 0) {
    qint64 result = getNodeSocket().readDatagram(incomingPacket.data(), incomingPacket.size(), address, port);

    SharedNodePointer sendingNode = sendingNodeForPacket(incomingPacket);
    if (sendingNode) {
        emit dataReceived(sendingNode->getType(), incomingPacket.size());
    } else {
        emit dataReceived(NodeType::Unassigned, incomingPacket.size());
    }

    return result;
}

qint64 LimitedNodeList::writeDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr) {
    // XXX can BandwidthRecorder be used for this?
    // stat collection for packets
    ++_numCollectedPackets;
    _numCollectedBytes += datagram.size();

    qint64 bytesWritten = _nodeSocket.writeDatagram(datagram,
                                                    destinationSockAddr.getAddress(), destinationSockAddr.getPort());

    if (bytesWritten < 0) {
        qCDebug(networking) << "ERROR in writeDatagram:" << _nodeSocket.error() << "-" << _nodeSocket.errorString();
    }

    return bytesWritten;
}

qint64 LimitedNodeList::writeDatagram(const QByteArray& datagram,
                                      const SharedNodePointer& destinationNode,
                                      const HifiSockAddr& overridenSockAddr) {
    if (destinationNode) {
        PacketType::Value packetType = packetTypeForPacket(datagram);

        if (NON_VERIFIED_PACKETS.contains(packetType)) {
            return writeUnverifiedDatagram(datagram, destinationNode, overridenSockAddr);
        }

        // if we don't have an overridden address, assume they want to send to the node's active socket
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

        QByteArray datagramCopy = datagram;

        // if we're here and the connection secret is null, debug out - this could be a problem
        if (destinationNode->getConnectionSecret().isNull()) {
            qDebug() << "LimitedNodeList::writeDatagram called for verified datagram with null connection secret for"
                << "destination node" << destinationNode->getUUID() << " - this is either not secure or will cause"
                << "this packet to be unverifiable on the receiving side.";
        }

        // perform replacement of hash and optionally also sequence number in the header
        if (SEQUENCE_NUMBERED_PACKETS.contains(packetType)) {
            PacketSequenceNumber sequenceNumber = getNextSequenceNumberForPacket(destinationNode->getUUID(), packetType);
            replaceHashAndSequenceNumberInPacket(datagramCopy, destinationNode->getConnectionSecret(),
                                                 sequenceNumber, packetType);
        } else {
            replaceHashInPacket(datagramCopy, destinationNode->getConnectionSecret(), packetType);
        }

        emit dataSent(destinationNode->getType(), datagram.size());
        auto bytesWritten = writeDatagram(datagramCopy, *destinationSockAddr);
        // Keep track of per-destination-node bandwidth
        destinationNode->recordBytesSent(bytesWritten);
        return bytesWritten;
    }

    // didn't have a destinationNode to send to, return 0
    return 0;
}

qint64 LimitedNodeList::writeUnverifiedDatagram(const QByteArray& datagram, const SharedNodePointer& destinationNode,
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

        PacketType::Value packetType = packetTypeForPacket(datagram);

        // optionally peform sequence number replacement in the header
        if (SEQUENCE_NUMBERED_PACKETS.contains(packetType)) {

            QByteArray datagramCopy = datagram;

            PacketSequenceNumber sequenceNumber = getNextSequenceNumberForPacket(destinationNode->getUUID(), packetType);
            replaceSequenceNumberInPacket(datagramCopy, sequenceNumber, packetType);

            // send the datagram with sequence number replaced in header
            return writeDatagram(datagramCopy, *destinationSockAddr);
        } else {
            return writeDatagram(datagram, *destinationSockAddr);
        }
    }

    // didn't have a destinationNode to send to, return 0
    return 0;
}

qint64 LimitedNodeList::writeUnverifiedDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr) {
    return writeDatagram(datagram, destinationSockAddr);
}

qint64 LimitedNodeList::writeDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                               const HifiSockAddr& overridenSockAddr) {
    return writeDatagram(QByteArray(data, size), destinationNode, overridenSockAddr);
}

qint64 LimitedNodeList::writeUnverifiedDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                               const HifiSockAddr& overridenSockAddr) {
    return writeUnverifiedDatagram(QByteArray(data, size), destinationNode, overridenSockAddr);
}

PacketSequenceNumber LimitedNodeList::getNextSequenceNumberForPacket(const QUuid& nodeUUID, PacketType::Value packetType) {
    // Thanks to std::map and std::unordered_map this line either default constructs the
    // PacketTypeSequenceMap and the PacketSequenceNumber or returns the existing value.
    // We use the postfix increment so that the stored value is incremented and the next
    // return gives the correct value.

    return _packetSequenceNumbers[nodeUUID][packetType]++;
}

void LimitedNodeList::processNodeData(const HifiSockAddr& senderSockAddr, const QByteArray& packet) {
    // the node decided not to do anything with this packet
    // if it comes from a known source we should keep that node alive
    SharedNodePointer matchingNode = sendingNodeForPacket(packet);
    if (matchingNode) {
        matchingNode->setLastHeardMicrostamp(usecTimestampNow());
    }
}

int LimitedNodeList::updateNodeWithDataFromPacket(const SharedNodePointer& matchingNode, const QByteArray &packet) {
    QMutexLocker locker(&matchingNode->getMutex());

    matchingNode->setLastHeardMicrostamp(usecTimestampNow());

    // if this was a sequence numbered packet we should store the last seq number for
    // a packet of this type for this node
    PacketType::Value packetType = packetTypeForPacket(packet);
    if (SEQUENCE_NUMBERED_PACKETS.contains(packetType)) {
        matchingNode->setLastSequenceNumberForPacketType(sequenceNumberFromHeader(packet, packetType), packetType);
    }

    NodeData* linkedData = matchingNode->getLinkedData();
    if (!linkedData && linkedDataCreateCallback) {
        linkedDataCreateCallback(matchingNode.data());
    }

    if (linkedData) {
        QMutexLocker linkedDataLocker(&linkedData->getMutex());
        return linkedData->parseData(packet);
    }
    return 0;
}

int LimitedNodeList::findNodeAndUpdateWithDataFromPacket(const QByteArray& packet) {
    SharedNodePointer matchingNode = sendingNodeForPacket(packet);

    if (matchingNode) {
        return updateNodeWithDataFromPacket(matchingNode, packet);
    }

    // we weren't able to match the sender address to the address we have for this node, unlock and don't parse
    return 0;
}

SharedNodePointer LimitedNodeList::nodeWithUUID(const QUuid& nodeUUID) {
    QReadLocker readLocker(&_nodeMutex);

    NodeHash::const_iterator it = _nodeHash.find(nodeUUID);
    return it == _nodeHash.cend() ? SharedNodePointer() : it->second;
 }

SharedNodePointer LimitedNodeList::sendingNodeForPacket(const QByteArray& packet) {
    QUuid nodeUUID = uuidFromPacketHeader(packet);

    // return the matching node, or NULL if there is no match
    return nodeWithUUID(nodeUUID);
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

void LimitedNodeList::processKillNode(const QByteArray& dataByteArray) {
    // read the node id
    QUuid nodeUUID = QUuid::fromRfc4122(dataByteArray.mid(numBytesForPacketHeader(dataByteArray), NUM_BYTES_RFC4122_UUID));

    // kill the node with this UUID, if it exists
    killNodeWithUUID(nodeUUID);
}

void LimitedNodeList::handleNodeKill(const SharedNodePointer& node) {
    qCDebug(networking) << "Killed" << *node;
    emit nodeKilled(node);
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

// unsigned LimitedNodeList::broadcastToNodes(PacketList& packetList, const NodeSet& destinationNodeTypes) {
    // unsigned n = 0;
    //
    // eachNode([&](const SharedNodePointer& node){
    //     if (destinationNodeTypes.contains(node->getType())) {
    //         writeDatagram(packet, node);
    //         ++n;
    //     }
    // });
    //
    // return n;
// }

NLPacket&& LimitedNodeList::constructPingPacket(PingType_t pingType) {
    int packetSize = sizeof(PingType_t) + sizeof(quint64);
    auto pingPacket { NLPacket::create(PacketType::Ping, packetSize); }

    QDataStream packetStream(&pingPacket.payload(), QIODevice::Append);

    packetStream << pingType;
    packetStream << usecTimestampNow();

    return pingPacket;
}

NLPacket&& LimitedNodeList::constructPingReplyPacket(const QByteArray& pingPacket) {
    QDataStream pingPacketStream(pingPacket);
    pingPacketStream.skipRawData(numBytesForPacketHeader(pingPacket));

    PingType_t typeFromOriginalPing;
    pingPacketStream >> typeFromOriginalPing;

    quint64 timeFromOriginalPing;
    pingPacketStream >> timeFromOriginalPing;

    int packetSize = sizeof(PingType_t) + sizeof(quint64) + sizeof(quint64);

    auto replyPacket { NLPacket::create(PacketType::Ping, packetSize); }

    QDataStream packetStream(&replyPacket, QIODevice::Append);
    packetStream << typeFromOriginalPing << timeFromOriginalPing << usecTimestampNow();

    return replyPacket;
}

NLPacket&& constructICEPingPacket(PingType_t pingType, const QUuid& iceID) {
    int packetSize = NUM_BYTES_RFC4122_UUID + sizeof(PingType_t);

    auto icePingPacket { NLPacket::create(PacketType::ICEPing, packetSize); }

    icePingPacket.payload().replace(0, NUM_BYTES_RFC4122_UUID, iceID.toRfc4122().data());
    memcpy(icePingPacket.payload() + NUM_BYTES_RFC4122_UUID, &pingType, sizeof(PingType_t));

    return icePingPacket;
}

NLPacket&& constructICEPingReplyPacket(const QByteArray& pingPacket, const QUuid& iceID) {
    // pull out the ping type so we can reply back with that
    PingType_t pingType;

    memcpy(&pingType, pingPacket.data() + NUM_BYTES_RFC4122_UUID, sizeof(PingType_t));

    int packetSize = NUM_BYTES_RFC4122_UUID + sizeof(PingType_t);
    auto icePingReplyPacket { NLPacket::create(PacketType::ICEPingReply, packetSize); }

    // pack the ICE ID and then the ping type
    memcpy(icePingReplyPacket.payload(), iceID.toRfc4122().data(), NUM_BYTES_RFC4122_UUID);
    memcpy(icePingReplyPacket.payload() + NUM_BYTES_RFC4122_UUID, &pingType, sizeof(PingType_t));

    return icePingReplyPacket;
}

SharedNodePointer LimitedNodeList::soloNodeOfType(char nodeType) {
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

    flagTimeForConnectionStep(ConnectionStep::SendSTUNRequest);

    _nodeSocket.writeDatagram((char*) stunRequestPacket, sizeof(stunRequestPacket),
                              _stunSockAddr.getAddress(), _stunSockAddr.getPort());
}

void LimitedNodeList::rebindNodeSocket() {
    quint16 oldPort = _nodeSocket.localPort();

    _nodeSocket.close();
    _nodeSocket.bind(QHostAddress::AnyIPv4, oldPort);
}

bool LimitedNodeList::processSTUNResponse(const QByteArray& packet) {
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

                    return true;
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

    return false;
}

void LimitedNodeList::startSTUNPublicSocketUpdate() {
    assert(!_initialSTUNTimer);

    if (!_initialSTUNTimer) {
        // if we don't know the STUN IP yet we need to have ourselves be called once it is known
        if (_stunSockAddr.getAddress().isNull()) {
            connect(&_stunSockAddr, &HifiSockAddr::lookupCompleted, this, &LimitedNodeList::startSTUNPublicSocketUpdate);

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
    sendPacketToIceServer(PacketTypeIceServerHeartbeat, iceServerSockAddr, _sessionUUID);
}

void LimitedNodeList::sendPeerQueryToIceServer(const HifiSockAddr& iceServerSockAddr, const QUuid& clientID,
                                               const QUuid& peerID) {
    sendPacketToIceServer(PacketTypeIceServerQuery, iceServerSockAddr, clientID, peerID);
}

void LimitedNodeList::sendPacketToIceServer(PacketType::Value packetType, const HifiSockAddr& iceServerSockAddr,
                                            const QUuid& headerID, const QUuid& peerID) {

    QByteArray iceRequestByteArray = byteArrayWithUUIDPopulatedHeader(packetType, headerID);
    QDataStream iceDataStream(&iceRequestByteArray, QIODevice::Append);

    iceDataStream << _publicSockAddr << _localSockAddr;

    if (packetType == PacketTypeIceServerQuery) {
        assert(!peerID.isNull());

        iceDataStream << peerID;

        qCDebug(networking) << "Sending packet to ICE server to request connection info for peer with ID"
            << uuidStringWithoutCurlyBraces(peerID);
    }

    writeUnverifiedDatagram(iceRequestByteArray, iceServerSockAddr);
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
