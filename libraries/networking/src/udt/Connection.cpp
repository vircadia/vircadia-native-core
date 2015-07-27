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

void Connection::sendACK() {
    static const int ACK_PACKET_PAYLOAD_BYTES = 8;
    
    // setup the ACK packet, make it static so we can re-use it
    static auto ackPacket = ControlPacket::create(ControlPacket::ACK, ACK_PACKET_PAYLOAD_BYTES);

    // have the send queue send off our packet
    _sendQueue->sendPacket(*ackPacket);
}

void Connection::sendLightACK() const {
    static const int LIGHT_ACK_PACKET_PAYLOAD_BYTES = 4;
    
    // create the light ACK packet, make it static so we can re-use it
    static auto lightACKPacket = ControlPacket::create(ControlPacket::ACK, LIGHT_ACK_PACKET_PAYLOAD_BYTES);
    
    SeqNum nextACKNumber = nextACK();
    
    if (nextACKNumber != _lastReceivedAcknowledgedACK) {
        // pack in the ACK
        memcpy(lightACKPacket->getPayload(), &nextACKNumber, sizeof(nextACKNumber));
        
        // have the send queue send off our packet
        _sendQueue->sendPacket(*lightACKPacket);
    }
}

SeqNum Connection::nextACK() const {
    // TODO: check if we have a loss list
    return _largestReceivedSeqNum + 1;
}

void Connection::processReceivedSeqNum(SeqNum seq) {
    if (udt::seqcmp(seq, _largestReceivedSeqNum + 1) > 0) {
        // TODO: Add range to loss list
        
        // TODO: Send loss report
    }
    
    if (seq > _largestReceivedSeqNum) {
        _largestReceivedSeqNum = seq;
    } else {
        // TODO: Remove seq from loss list
    }
}

void Connection::processControl(std::unique_ptr<ControlPacket> controlPacket) {
    switch (controlPacket->getType()) {
        case ControlPacket::ACK:
            break;
        case ControlPacket::ACK2:
            break;
        case ControlPacket::NAK:
            break;
        case ControlPacket::PacketPair:
            break;
    }
}
