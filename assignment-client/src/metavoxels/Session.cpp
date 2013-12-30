//
//  Session.cpp
//  hifi
//
//  Created by Andrzej Kapolka on 12/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "MetavoxelServer.h"
#include "Session.h"

Session::Session(MetavoxelServer* server, const QUuid& sessionId, const QByteArray& datagramHeader) :
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

void Session::receivedData(const QByteArray& data, const HifiSockAddr& sender) {
    // reset the timeout timer
    _timeoutTimer.start();

    // save the most recent sender
    _sender = sender;
    
    // process through sequencer
    _sequencer.receivedDatagram(data);
}

void Session::timedOut() {
    qDebug() << "Session timed out [sessionId=" << _sessionId << ", sender=" << _sender << "]\n";
    _server->removeSession(_sessionId);
}

void Session::sendData(const QByteArray& data) {
    NodeList::getInstance()->getNodeSocket().writeDatagram(data, _sender.getAddress(), _sender.getPort());
}

void Session::readPacket(Bitstream& in) {
}
