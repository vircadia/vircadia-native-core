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
#include "LimitedNodeList.h"
#include "PacketHeaders.h"
#include "SharedUtil.h"
#include "UUID.h"

const char SOLO_NODE_TYPES[2] = {
    NodeType::AvatarMixer,
    NodeType::AudioMixer
};

const QUrl DEFAULT_NODE_AUTH_URL = QUrl("https://data.highfidelity.io");

LimitedNodeList* LimitedNodeList::_sharedInstance = NULL;

LimitedNodeList* LimitedNodeList::createInstance(unsigned short socketListenPort, unsigned short dtlsPort) {
    if (!_sharedInstance) {
        NodeType::init();
        
        _sharedInstance = new LimitedNodeList(socketListenPort, dtlsPort);

        // register the SharedNodePointer meta-type for signals/slots
        qRegisterMetaType<SharedNodePointer>();
    } else {
        qDebug("LimitedNodeList createInstance called with existing instance.");
    }

    return _sharedInstance;
}

LimitedNodeList* LimitedNodeList::getInstance() {
    if (!_sharedInstance) {
        qDebug("LimitedNodeList getInstance called before call to createInstance. Returning NULL pointer.");
    }

    return _sharedInstance;
}


LimitedNodeList::LimitedNodeList(unsigned short socketListenPort, unsigned short dtlsListenPort) :
    _sessionUUID(),
    _nodeHash(),
    _nodeHashMutex(QMutex::Recursive),
    _nodeSocket(this),
    _dtlsSocket(NULL),
    _numCollectedPackets(0),
    _numCollectedBytes(0),
    _packetStatTimer()
{
    _nodeSocket.bind(QHostAddress::AnyIPv4, socketListenPort);
    qDebug() << "NodeList socket is listening on" << _nodeSocket.localPort();
    
    if (dtlsListenPort > 0) {
        // only create the DTLS socket during constructor if a custom port is passed
        _dtlsSocket = new QUdpSocket(this);
        
        _dtlsSocket->bind(QHostAddress::AnyIPv4, dtlsListenPort);
        qDebug() << "NodeList DTLS socket is listening on" << _dtlsSocket->localPort();
    }
    
    const int LARGER_SNDBUF_SIZE = 1048576;
    changeSendSocketBufferSize(LARGER_SNDBUF_SIZE);
    
    _packetStatTimer.start();
}

void LimitedNodeList::setSessionUUID(const QUuid& sessionUUID) {
    QUuid oldUUID = _sessionUUID;
    _sessionUUID = sessionUUID;
    
    if (sessionUUID != oldUUID) {
        qDebug() << "NodeList UUID changed from" <<  uuidStringWithoutCurlyBraces(oldUUID)
        << "to" << uuidStringWithoutCurlyBraces(_sessionUUID);
        emit uuidChanged(sessionUUID, oldUUID);
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
        
#if defined(IP_DONTFRAG) || defined(IP_MTU_DISCOVER)
        qDebug() << "Making required DTLS changes to LimitedNodeList DTLS socket.";
        
        int socketHandle = _dtlsSocket->socketDescriptor();
#if defined(IP_DONTFRAG)
        int optValue = 1;
        setsockopt(socketHandle, IPPROTO_IP, IP_DONTFRAG, reinterpret_cast<const void*>(&optValue), sizeof(optValue));
#elif defined(IP_MTU_DISCOVER)
        int optValue = 1;
        setsockopt(socketHandle, IPPROTO_IP, IP_MTU_DISCOVER, reinterpret_cast<const void*>(&optValue), sizeof(optValue));
#endif
#endif
        
        qDebug() << "LimitedNodeList DTLS socket is listening on" << _dtlsSocket->localPort();
    }
    
    return *_dtlsSocket;
}

void LimitedNodeList::changeSendSocketBufferSize(int numSendBytes) {
    // change the socket send buffer size to be 1MB
    int oldBufferSize = 0;
    
#ifdef Q_OS_WIN
    int sizeOfInt = sizeof(oldBufferSize);
#else
    unsigned int sizeOfInt = sizeof(oldBufferSize);
#endif
    
    getsockopt(_nodeSocket.socketDescriptor(), SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&oldBufferSize), &sizeOfInt);
    
    setsockopt(_nodeSocket.socketDescriptor(), SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&numSendBytes),
               sizeof(numSendBytes));
    
    int newBufferSize = 0;
    getsockopt(_nodeSocket.socketDescriptor(), SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&newBufferSize), &sizeOfInt);
    
    qDebug() << "Changed socket send buffer size from" << oldBufferSize << "to" << newBufferSize << "bytes";
}

bool LimitedNodeList::packetVersionAndHashMatch(const QByteArray& packet) {
    PacketType checkType = packetTypeForPacket(packet);
    int numPacketTypeBytes = numBytesArithmeticCodingFromBuffer(packet.data());
    
    if (packet[numPacketTypeBytes] != versionForPacketType(checkType)
        && checkType != PacketTypeStunResponse) {
        PacketType mismatchType = packetTypeForPacket(packet);
        
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
            qDebug() << "Packet of type" << checkType << "received from unknown node with UUID"
                << uuidFromPacketHeader(packet);
        }
    } else {
        return true;
    }
    
    return false;
}

qint64 LimitedNodeList::writeDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr,
                                      const QUuid& connectionSecret) {
    QByteArray datagramCopy = datagram;
    
    if (!connectionSecret.isNull()) {
        // setup the MD5 hash for source verification in the header
        replaceHashInPacketGivenConnectionUUID(datagramCopy, connectionSecret);
    }
    
    // stat collection for packets
    ++_numCollectedPackets;
    _numCollectedBytes += datagram.size();
    
    qint64 bytesWritten = _nodeSocket.writeDatagram(datagramCopy,
                                                    destinationSockAddr.getAddress(), destinationSockAddr.getPort());
    
    if (bytesWritten < 0) {
        qDebug() << "ERROR in writeDatagram:" << _nodeSocket.error() << "-" << _nodeSocket.errorString();
    }
    
    return bytesWritten;
}

qint64 LimitedNodeList::writeDatagram(const QByteArray& datagram, const SharedNodePointer& destinationNode,
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
        
        return writeDatagram(datagram, *destinationSockAddr, destinationNode->getConnectionSecret());
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
        
        // don't use the node secret!
        return writeDatagram(datagram, *destinationSockAddr, QUuid());
    }
    
    // didn't have a destinationNode to send to, return 0
    return 0;
}

qint64 LimitedNodeList::writeUnverifiedDatagram(const QByteArray& datagram, const HifiSockAddr& destinationSockAddr) {
    return writeDatagram(datagram, destinationSockAddr, QUuid());
}

qint64 LimitedNodeList::writeDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                               const HifiSockAddr& overridenSockAddr) {
    return writeDatagram(QByteArray(data, size), destinationNode, overridenSockAddr);
}

qint64 LimitedNodeList::writeUnverifiedDatagram(const char* data, qint64 size, const SharedNodePointer& destinationNode,
                               const HifiSockAddr& overridenSockAddr) {
    return writeUnverifiedDatagram(QByteArray(data, size), destinationNode, overridenSockAddr);
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
    matchingNode->recordBytesReceived(packet.size());
    
    if (!matchingNode->getLinkedData() && linkedDataCreateCallback) {
        linkedDataCreateCallback(matchingNode.data());
    }
    
    QMutexLocker linkedDataLocker(&matchingNode->getLinkedData()->getMutex());
    
    return matchingNode->getLinkedData()->parseData(packet);
}

int LimitedNodeList::findNodeAndUpdateWithDataFromPacket(const QByteArray& packet) {
    SharedNodePointer matchingNode = sendingNodeForPacket(packet);
    
    if (matchingNode) {
        updateNodeWithDataFromPacket(matchingNode, packet);
    }
    
    // we weren't able to match the sender address to the address we have for this node, unlock and don't parse
    return 0;
}

SharedNodePointer LimitedNodeList::nodeWithUUID(const QUuid& nodeUUID, bool blockingLock) {
    const int WAIT_TIME = 10; // wait up to 10ms in the try lock case
    SharedNodePointer node;
    // if caller wants us to block and guarantee the correct answer, then honor that request
    if (blockingLock) {
        // this will block till we can get access
        QMutexLocker locker(&_nodeHashMutex);
        node = _nodeHash.value(nodeUUID);
    } else if (_nodeHashMutex.tryLock(WAIT_TIME)) { // some callers are willing to get wrong answers but not block
        node = _nodeHash.value(nodeUUID);
        _nodeHashMutex.unlock();
    }
    return node;
 }

SharedNodePointer LimitedNodeList::sendingNodeForPacket(const QByteArray& packet) {
    QUuid nodeUUID = uuidFromPacketHeader(packet);
    
    // return the matching node, or NULL if there is no match
    return nodeWithUUID(nodeUUID);
}

NodeHash LimitedNodeList::getNodeHash() {
    QMutexLocker locker(&_nodeHashMutex);
    return NodeHash(_nodeHash);
}

void LimitedNodeList::eraseAllNodes() {
    qDebug() << "Clearing the NodeList. Deleting all nodes in list.";
    
    QMutexLocker locker(&_nodeHashMutex);

    NodeHash::iterator nodeItem = _nodeHash.begin();

    // iterate the nodes in the list
    while (nodeItem != _nodeHash.end()) {
        nodeItem = killNodeAtHashIterator(nodeItem);
    }
}

void LimitedNodeList::reset() {
    eraseAllNodes();
}

void LimitedNodeList::killNodeWithUUID(const QUuid& nodeUUID) {
    QMutexLocker locker(&_nodeHashMutex);
    
    NodeHash::iterator nodeItemToKill = _nodeHash.find(nodeUUID);
    if (nodeItemToKill != _nodeHash.end()) {
        killNodeAtHashIterator(nodeItemToKill);
    }
}

NodeHash::iterator LimitedNodeList::killNodeAtHashIterator(NodeHash::iterator& nodeItemToKill) {
    qDebug() << "Killed" << *nodeItemToKill.value();
    emit nodeKilled(nodeItemToKill.value());
    return _nodeHash.erase(nodeItemToKill);
}

void LimitedNodeList::processKillNode(const QByteArray& dataByteArray) {
    // read the node id
    QUuid nodeUUID = QUuid::fromRfc4122(dataByteArray.mid(numBytesForPacketHeader(dataByteArray), NUM_BYTES_RFC4122_UUID));

    // kill the node with this UUID, if it exists
    killNodeWithUUID(nodeUUID);
}

SharedNodePointer LimitedNodeList::addOrUpdateNode(const QUuid& uuid, NodeType_t nodeType,
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

SharedNodePointer LimitedNodeList::updateSocketsForNode(const QUuid& uuid,
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

unsigned LimitedNodeList::broadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes) {
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

SharedNodePointer LimitedNodeList::soloNodeOfType(char nodeType) {

    if (memchr(SOLO_NODE_TYPES, nodeType, sizeof(SOLO_NODE_TYPES))) {
        foreach (const SharedNodePointer& node, getNodeHash()) {
            if (node->getType() == nodeType) {
                return node;
            }
        }
    }
    return SharedNodePointer();
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

    _nodeHashMutex.lock();
    
    NodeHash::iterator nodeItem = _nodeHash.begin();

    while (nodeItem != _nodeHash.end()) {
        SharedNodePointer node = nodeItem.value();

        node->getMutex().lock();

        if ((usecTimestampNow() - node->getLastHeardMicrostamp()) > (NODE_SILENCE_THRESHOLD_MSECS * 1000)) {
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
