//
//  MetavoxelServer.cpp
//  hifi
//
//  Created by Andrzej Kapolka on 12/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <PacketHeaders.h>

#include <MetavoxelMessages.h>
#include <MetavoxelUtil.h>

#include "MetavoxelServer.h"

MetavoxelServer::MetavoxelServer(const unsigned char* dataBuffer, int numBytes) :
    ThreadedAssignment(dataBuffer, numBytes) {
}

void MetavoxelServer::removeSession(const QUuid& sessionId) {
    delete _sessions.take(sessionId);
}

void MetavoxelServer::run() {
    commonInit("metavoxel-server", NODE_TYPE_METAVOXEL_SERVER);
}
    
void MetavoxelServer::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    switch (dataByteArray.at(0)) {
        case PACKET_TYPE_METAVOXEL_DATA:
            processData(dataByteArray, senderSockAddr);
            break;
        
        default:
            NodeList::getInstance()->processNodeData(senderSockAddr, (unsigned char*)dataByteArray.data(), dataByteArray.size());
            break;
    }
}

void MetavoxelServer::processData(const QByteArray& data, const HifiSockAddr& sender) {
    // read the session id
    int headerPlusIDSize;
    QUuid sessionID = readSessionID(data, sender, headerPlusIDSize);
    if (sessionID.isNull()) {
        return;
    }
    
    // forward to session, creating if necessary
    MetavoxelSession*& session = _sessions[sessionID];
    if (!session) {
        session = new MetavoxelSession(this, sessionID, QByteArray::fromRawData(data.constData(), headerPlusIDSize));
    }
    session->receivedData(data, sender);
}

MetavoxelSession::MetavoxelSession(MetavoxelServer* server, const QUuid& sessionId, const QByteArray& datagramHeader) :
    QObject(server),
    _server(server),
    _sessionId(sessionId),
    _sequencer(datagramHeader) {
    
    const int TIMEOUT_INTERVAL = 30 * 1000;
    _timeoutTimer.setInterval(TIMEOUT_INTERVAL);
    _timeoutTimer.setSingleShot(true);
    connect(&_timeoutTimer, SIGNAL(timeout()), SLOT(timedOut()));
    
    connect(&_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendData(const QByteArray&)));
    connect(&_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readPacket(Bitstream&)));
}

void MetavoxelSession::receivedData(const QByteArray& data, const HifiSockAddr& sender) {
    // reset the timeout timer
    _timeoutTimer.start();

    // save the most recent sender
    _sender = sender;
    
    // process through sequencer
    _sequencer.receivedDatagram(data);
}

void MetavoxelSession::timedOut() {
    qDebug() << "Session timed out [sessionId=" << _sessionId << ", sender=" << _sender << "]\n";
    _server->removeSession(_sessionId);
}

void MetavoxelSession::sendData(const QByteArray& data) {
    NodeList::getInstance()->getNodeSocket().writeDatagram(data, _sender.getAddress(), _sender.getPort());
}

void MetavoxelSession::readPacket(Bitstream& in) {
    QVariant msg;
    in >> msg;
    glm::vec3 position = msg.value<ClientStateMessage>().position;
    qDebug() << position.x << " " << position.y << " " << position.z << "\n";

    Bitstream& out = _sequencer.startPacket();
    _sequencer.endPacket();
}
