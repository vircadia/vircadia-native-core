//
//  MetavoxelServer.cpp
//  hifi
//
//  Created by Andrzej Kapolka on 12/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QDateTime>

#include <PacketHeaders.h>

#include <MetavoxelMessages.h>
#include <MetavoxelUtil.h>

#include "MetavoxelServer.h"

const int SEND_INTERVAL = 50;

MetavoxelServer::MetavoxelServer(const QByteArray& packet) :
    ThreadedAssignment(packet) {
    
    _sendTimer.setSingleShot(true);
    connect(&_sendTimer, SIGNAL(timeout()), SLOT(sendDeltas()));
}

void MetavoxelServer::applyEdit(const MetavoxelEditMessage& edit) {
    edit.apply(_data, SharedObject::getWeakHash());
}

const QString METAVOXEL_SERVER_LOGGING_NAME = "metavoxel-server";

void MetavoxelServer::run() {
    commonInit(METAVOXEL_SERVER_LOGGING_NAME, NodeType::MetavoxelServer);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
    
    connect(nodeList, SIGNAL(nodeAdded(SharedNodePointer)), SLOT(maybeAttachSession(const SharedNodePointer&)));
    
    _lastSend = QDateTime::currentMSecsSinceEpoch();
    _sendTimer.start(SEND_INTERVAL);
}

void MetavoxelServer::readPendingDatagrams() {
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    
    NodeList* nodeList = NodeList::getInstance();
    
    while (readAvailableDatagram(receivedPacket, senderSockAddr)) {
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            switch (packetTypeForPacket(receivedPacket)) {
                case PacketTypeMetavoxelData:
                    nodeList->findNodeAndUpdateWithDataFromPacket(receivedPacket);
                    break;
                
                default:
                    nodeList->processNodeData(senderSockAddr, receivedPacket);
                    break;
            }
        }
    }
}

void MetavoxelServer::maybeAttachSession(const SharedNodePointer& node) {
    if (node->getType() == NodeType::Agent) {
        QMutexLocker locker(&node->getMutex());
        node->setLinkedData(new MetavoxelSession(this, NodeList::getInstance()->nodeWithUUID(node->getUUID())));
    }
}

void MetavoxelServer::sendDeltas() {
    // send deltas for all sessions
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::Agent) {
            static_cast<MetavoxelSession*>(node->getLinkedData())->sendDelta();
        }
    }
    
    // restart the send timer
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int elapsed = now - _lastSend;
    _lastSend = now;
    
    _sendTimer.start(qMax(0, 2 * SEND_INTERVAL - elapsed));
}

MetavoxelSession::MetavoxelSession(MetavoxelServer* server, const SharedNodePointer& node) :
    _server(server),
    _sequencer(byteArrayWithPopulatedHeader(PacketTypeMetavoxelData)),
    _node(node) {
    
    connect(&_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendData(const QByteArray&)));
    connect(&_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readPacket(Bitstream&)));
    connect(&_sequencer, SIGNAL(sendAcknowledged(int)), SLOT(clearSendRecordsBefore(int)));
    connect(&_sequencer, SIGNAL(receivedHighPriorityMessage(const QVariant&)), SLOT(handleMessage(const QVariant&)));
    
    // insert the baseline send record
    SendRecord record = { 0 };
    _sendRecords.append(record);
}

MetavoxelSession::~MetavoxelSession() {
}

int MetavoxelSession::parseData(const QByteArray& packet) {
    // process through sequencer
    _sequencer.receivedDatagram(packet);
    return packet.size();
}

void MetavoxelSession::sendDelta() {
    // wait until we have a valid lod
    if (!_lod.isValid()) {
        return;
    }
    Bitstream& out = _sequencer.startPacket();
    out << QVariant::fromValue(MetavoxelDeltaMessage());
    _server->getData().writeDelta(_sendRecords.first().data, _sendRecords.first().lod, out, _lod);
    _sequencer.endPacket();
    
    // record the send
    SendRecord record = { _sequencer.getOutgoingPacketNumber(), _server->getData(), _lod };
    _sendRecords.append(record);
}

void MetavoxelSession::sendData(const QByteArray& data) {
    NodeList::getInstance()->writeDatagram(data, _node);
}

void MetavoxelSession::readPacket(Bitstream& in) {
    QVariant message;
    in >> message;
    handleMessage(message);
}

void MetavoxelSession::clearSendRecordsBefore(int index) {
    _sendRecords.erase(_sendRecords.begin(), _sendRecords.begin() + index + 1);
}

void MetavoxelSession::handleMessage(const QVariant& message) {
    int userType = message.userType();
    if (userType == ClientStateMessage::Type) {
        ClientStateMessage state = message.value<ClientStateMessage>();
        _lod = state.lod;
    
    } else if (userType == MetavoxelEditMessage::Type) {
        _server->applyEdit(message.value<MetavoxelEditMessage>());
        
    } else if (userType == QMetaType::QVariantList) {
        foreach (const QVariant& element, message.toList()) {
            handleMessage(element);
        }
    }
}
