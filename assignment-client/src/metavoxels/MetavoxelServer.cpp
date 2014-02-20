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
    edit.apply(_data);
}

void MetavoxelServer::removeSession(const QUuid& sessionId) {
    _sessions.take(sessionId)->deleteLater();
}

const char METAVOXEL_SERVER_LOGGING_NAME[] = "metavoxel-server";

void MetavoxelServer::run() {
    commonInit(METAVOXEL_SERVER_LOGGING_NAME, NodeType::MetavoxelServer);
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);
    
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
                case PacketTypeMetavoxelData: {
                    SharedNodePointer matchingNode = nodeList->sendingNodeForPacket(receivedPacket);
                    if (matchingNode) {
                        processData(receivedPacket, matchingNode);
                    }
                    break;
                }
                default:
                    NodeList::getInstance()->processNodeData(senderSockAddr, receivedPacket);
                    break;
            }
        }
    }
}

void MetavoxelServer::sendDeltas() {
    // send deltas for all sessions
    foreach (MetavoxelSession* session, _sessions) {
        session->sendDelta();
    }
    
    // restart the send timer
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    int elapsed = now - _lastSend;
    _lastSend = now;
    
    _sendTimer.start(qMax(0, 2 * SEND_INTERVAL - elapsed));
}

void MetavoxelServer::processData(const QByteArray& data, const SharedNodePointer& sendingNode) {
    // read the session id
    int headerPlusIDSize;
    QUuid sessionID = readSessionID(data, sendingNode, headerPlusIDSize);
    if (sessionID.isNull()) {
        return;
    }
    
    // forward to session, creating if necessary
    MetavoxelSession*& session = _sessions[sessionID];
    if (!session) {
        session = new MetavoxelSession(this, sessionID, QByteArray::fromRawData(data.constData(), headerPlusIDSize),
                                       sendingNode);
    }
    session->receivedData(data, sendingNode);
}

MetavoxelSession::MetavoxelSession(MetavoxelServer* server, const QUuid& sessionId,
        const QByteArray& datagramHeader, const SharedNodePointer& sendingNode) :
    QObject(server),
    _server(server),
    _sessionId(sessionId),
    _sequencer(datagramHeader),
    _sendingNode(sendingNode) {
    
    const int TIMEOUT_INTERVAL = 30 * 1000;
    _timeoutTimer.setInterval(TIMEOUT_INTERVAL);
    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, SIGNAL(timeout()), SLOT(timedOut()));
    
    connect(&_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendData(const QByteArray&)));
    connect(&_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readPacket(Bitstream&)));
    connect(&_sequencer, SIGNAL(sendAcknowledged(int)), SLOT(clearSendRecordsBefore(int)));
    connect(&_sequencer, SIGNAL(receivedHighPriorityMessage(const QVariant&)), SLOT(handleMessage(const QVariant&)));
    
    // insert the baseline send record
    SendRecord record = { 0 };
    _sendRecords.append(record);
    
    qDebug() << "Opened session [sessionId=" << _sessionId << ", sendingNode=" << sendingNode << "]";
}

void MetavoxelSession::receivedData(const QByteArray& data, const SharedNodePointer& sendingNode) {
    // reset the timeout timer
    _timeoutTimer.start();

    // save the most recent sender
    _sendingNode = sendingNode;
    
    // process through sequencer
    _sequencer.receivedDatagram(data);
}

void MetavoxelSession::sendDelta() {
    Bitstream& out = _sequencer.startPacket();
    out << QVariant::fromValue(MetavoxelDeltaMessage());
    _server->getData().writeDelta(_sendRecords.first().data, out);
    _sequencer.endPacket();
    
    // record the send
    SendRecord record = { _sequencer.getOutgoingPacketNumber(), _server->getData() };
    _sendRecords.append(record);
}

void MetavoxelSession::timedOut() {
    qDebug() << "Session timed out [sessionId=" << _sessionId << ", sendingNode=" << _sendingNode << "]";
    _server->removeSession(_sessionId);
}

void MetavoxelSession::sendData(const QByteArray& data) {
    NodeList::getInstance()->writeDatagram(data, _sendingNode);
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
    if (userType == CloseSessionMessage::Type) {
        qDebug() << "Session closed [sessionId=" << _sessionId << ", sendingNode=" << _sendingNode << "]";
        _server->removeSession(_sessionId);
    
    } else if (userType == ClientStateMessage::Type) {
        ClientStateMessage state = message.value<ClientStateMessage>();
        _position = state.position;
    
    } else if (userType == MetavoxelEditMessage::Type) {
        _server->applyEdit(message.value<MetavoxelEditMessage>());
        
    } else if (userType == QMetaType::QVariantList) {
        foreach (const QVariant& element, message.toList()) {
            handleMessage(element);
        }
    }
}
