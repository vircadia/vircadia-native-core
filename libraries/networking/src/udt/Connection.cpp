//
//  Connection.cpp
//
//
//  Created by Clement on 7/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Connection.h"

#include "../HifiSockAddr.h"
#include "ControlPacket.h"
#include "Packet.h"
#include "Socket.h"

using namespace udt;

Connection::Connection(Socket* parentSocket, HifiSockAddr destination) {
    
}

void Connection::send(std::unique_ptr<Packet> packet) {
    if (_sendQueue) {
        _sendQueue->queuePacket(std::move(packet));
    }
}

void Connection::processReceivedSeqNum(SeqNum seq) {
    if (udt::seqcmp(seq, _largestRecievedSeqNum + 1) > 0) {
        // TODO: Add range to loss list
        
        // TODO: Send loss report
    }
    
    if (seq > _largestRecievedSeqNum) {
        _largestRecievedSeqNum = seq;
    } else {
        // TODO: Remove seq from loss list
    }
}

void Connection::processControl(std::unique_ptr<ControlPacket> controlPacket) {
    switch (controlPacket->getType()) {
        case ControlPacket::Type::ACK:
            break;
        case ControlPacket::Type::ACK2:
            break;
        case ControlPacket::Type::NAK:
            break;
        case ControlPacket::Type::PacketPair:
            break;
    }
}
