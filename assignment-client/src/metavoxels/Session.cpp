//
//  Session.cpp
//  hifi
//
//  Created by Andrzej Kapolka on 12/28/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "MetavoxelServer.h"
#include "Session.h"

Session::Session(MetavoxelServer* server) :
    QObject(server),
    _server(server) {
    
    connect(&_sequencer, SIGNAL(readyToWrite(const QByteArray&)), SLOT(sendData(const QByteArray&)));
    connect(&_sequencer, SIGNAL(readyToRead(Bitstream&)), SLOT(readPacket(Bitstream&)));
}

void Session::receivedData(const QByteArray& data, const HifiSockAddr& sender) {
    // process through sequencer
    _sequencer.receivedDatagram(data);
}

void Session::sendData(const QByteArray& data) {
}

void Session::readPacket(Bitstream& in) {
}
