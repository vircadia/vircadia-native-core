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
using namespace std;

Connection::Connection(Socket* parentSocket, HifiSockAddr destination) {
    
}

void Connection::send(unique_ptr<Packet> packet) {
    if (_sendQueue) {
        _sendQueue->queuePacket(move(packet));
    }
}

void Connection::processReceivedSeqNum(SeqNum seq) {
    // If this is not the next sequence number, report loss
    if (seq > _largestRecievedSeqNum + 1) {
        if (_largestRecievedSeqNum + 1 == seq - 1) {
            _lossList.append(_largestRecievedSeqNum + 1);
        } else {
            _lossList.append(_largestRecievedSeqNum + 1, seq - 1);
        }
        
        // TODO: Send loss report
    }
    
    if (seq > _largestRecievedSeqNum) {
        // Update largest recieved sequence number
        _largestRecievedSeqNum = seq;
    } else {
        // Otherwise, it's a resend, remove it from the loss list
        _lossList.remove(seq);
    }
}

void Connection::processControl(unique_ptr<ControlPacket> controlPacket) {
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
