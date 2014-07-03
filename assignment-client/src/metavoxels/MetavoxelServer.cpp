//
//  MetavoxelServer.cpp
//  assignment-client/src/metavoxels
//
//  Created by Andrzej Kapolka on 12/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
        node->setLinkedData(new MetavoxelSession(node, this));
    }
}

void MetavoxelServer::sendDeltas() {
    // send deltas for all sessions
    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
        if (node->getType() == NodeType::Agent) {
            static_cast<MetavoxelSession*>(node->getLinkedData())->update();
        }
    }
    
    // restart the send timer
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int elapsed = now - _lastSend;
    _lastSend = now;
    
    _sendTimer.start(qMax(0, 2 * SEND_INTERVAL - elapsed));
}

MetavoxelSession::MetavoxelSession(const SharedNodePointer& node, MetavoxelServer* server) :
    Endpoint(node, new PacketRecord(), NULL),
    _server(server) {
    
    connect(&_sequencer, SIGNAL(receivedHighPriorityMessage(const QVariant&)), SLOT(handleMessage(const QVariant&)));
    connect(_sequencer.getReliableInputChannel(), SIGNAL(receivedMessage(const QVariant&, Bitstream&)),
        SLOT(handleMessage(const QVariant&, Bitstream&)));
}

void MetavoxelSession::update() {
    // wait until we have a valid lod
    if (_lod.isValid()) {
        Endpoint::update();
    }
}

void MetavoxelSession::writeUpdateMessage(Bitstream& out) {
    out << QVariant::fromValue(MetavoxelDeltaMessage());
    PacketRecord* sendRecord = getLastAcknowledgedSendRecord();
    _server->getData().writeDelta(sendRecord->getData(), sendRecord->getLOD(), out, _lod);
}

void MetavoxelSession::handleMessage(const QVariant& message, Bitstream& in) {
    handleMessage(message);
}

PacketRecord* MetavoxelSession::maybeCreateSendRecord() const {
    return new PacketRecord(_lod, _server->getData());
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
