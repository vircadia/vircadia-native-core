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
using namespace std::chrono;

Connection::Connection(Socket* parentSocket, HifiSockAddr destination) {
    
}

void Connection::send(unique_ptr<Packet> packet) {
    if (_sendQueue) {
        _sendQueue->queuePacket(move(packet));
    }
}

void Connection::sendACK(bool wasCausedBySyncTimeout) {
    static const int ACK_PACKET_PAYLOAD_BYTES = sizeof(_lastSentACK) + sizeof(_currentACKSubSequenceNumber)
        + sizeof(_rtt) + sizeof(_rttVariance) + sizeof(int32_t) + sizeof(int32_t);
    
    // setup the ACK packet, make it static so we can re-use it
    static auto ackPacket = ControlPacket::create(ControlPacket::ACK, ACK_PACKET_PAYLOAD_BYTES);
    ackPacket->reset(); // We need to reset it every time.
    
    auto currentTime = high_resolution_clock::now();
    
    SeqNum nextACKNumber = nextACK();
    
    if (nextACKNumber <= _lastReceivedAcknowledgedACK) {
        // we already got an ACK2 for this ACK we would be sending, don't bother
        return;
    }
    
    if (nextACKNumber >= _lastSentACK) {
        // we have received new packets since the last sent ACK
        
        // update the last sent ACK
        _lastSentACK = nextACKNumber;
        
        // remove the ACKed packets from the receive queue
        
    } else if (nextACKNumber == _lastSentACK) {
        // We already sent this ACK, but check if we should re-send it.
        // We will re-send if it has been more than RTT + (4 * RTT variance) since the last ACK
        milliseconds sinceLastACK = duration_cast<milliseconds>(currentTime - _lastACKTime);
        if (sinceLastACK.count() < (_rtt + (4 * _rttVariance))) {
            return;
        }
    }
    
    // pack in the ACK sub-sequence number
    ackPacket->writePrimitive(_currentACKSubSequenceNumber++);
    
    // pack in the ACK number
    ackPacket->writePrimitive(nextACKNumber);
    
    // pack in the RTT and variance
    ackPacket->writePrimitive(_rtt);
    ackPacket->writePrimitive(_rttVariance);
    
    // pack the available buffer size - must be a minimum of 2
    
    if (wasCausedBySyncTimeout) {
        // pack in the receive speed and bandwidth
        
        // record this as the last ACK send time
        _lastACKTime = high_resolution_clock::now();
    }
    
    // have the send queue send off our packet
    _sendQueue->sendPacket(*ackPacket);
}

void Connection::sendLightACK() const {
    // create the light ACK packet, make it static so we can re-use it
    static const int LIGHT_ACK_PACKET_PAYLOAD_BYTES = sizeof(SeqNum);
    static auto lightACKPacket = ControlPacket::create(ControlPacket::ACK, LIGHT_ACK_PACKET_PAYLOAD_BYTES);
    
    SeqNum nextACKNumber = nextACK();
    
    if (nextACKNumber == _lastReceivedAcknowledgedACK) {
        // we already got an ACK2 for this ACK we would be sending, don't bother
        return;
    }
    
    // reset the lightACKPacket before we go to write the ACK to it
    lightACKPacket->reset();
    
    // pack in the ACK
    lightACKPacket->writePrimitive(nextACKNumber);
    
    // have the send queue send off our packet immediately
    _sendQueue->sendPacket(*lightACKPacket);
}

SeqNum Connection::nextACK() const {
    if (_lossList.getLength() > 0) {
        return _lossList.getFirstSeqNum();
    } else {
        return _largestReceivedSeqNum + 1;
    }
}

void Connection::processReceivedSeqNum(SeqNum seq) {
    // If this is not the next sequence number, report loss
    if (seq > _largestReceivedSeqNum + 1) {
        if (_largestReceivedSeqNum + 1 == seq - 1) {
            _lossList.append(_largestReceivedSeqNum + 1);
        } else {
            _lossList.append(_largestReceivedSeqNum + 1, seq - 1);
        }
        
        // create the loss report packet, make it static so we can re-use it
        static const int NAK_PACKET_PAYLOAD_BYTES = 2 * sizeof(SeqNum);
        static auto lossReport = ControlPacket::create(ControlPacket::NAK, NAK_PACKET_PAYLOAD_BYTES);
        lossReport->reset(); // We need to reset it every time.
        
        // pack in the loss report
        lossReport->writePrimitive(_largestReceivedSeqNum + 1);
        if (_largestReceivedSeqNum + 1 != seq - 1) {
            lossReport->writePrimitive(seq - 1);
        }
        
        // have the send queue send off our packet immediately
        _sendQueue->sendPacket(*lossReport);
    }
    
    if (seq > _largestReceivedSeqNum) {
        // Update largest recieved sequence number
        _largestReceivedSeqNum = seq;
    } else {
        // Otherwise, it's a resend, remove it from the loss list
        _lossList.remove(seq);
    }
}

void Connection::processControl(unique_ptr<ControlPacket> controlPacket) {
    switch (controlPacket->getType()) {
        case ControlPacket::ACK: {
            processACK(move(controlPacket));
            break;
        }
        case ControlPacket::ACK2: {
            processACK2(move(controlPacket));
            break;
        }
        case ControlPacket::NAK: {
            processNAK(move(controlPacket));
            break;
        }
        case ControlPacket::PacketPair: {
            
            break;
        }
    }
}

void Connection::processACK(std::unique_ptr<ControlPacket> controlPacket) {
    // read the ACK sub-sequence number
    SeqNum currentACKSubSequenceNumber;
    controlPacket->readPrimitive(&currentACKSubSequenceNumber);
    
    // read the ACK number
    SeqNum nextACKNumber;
    controlPacket->readPrimitive(&nextACKNumber);
    
    // read the RTT and variance
    int32_t rtt, rttVariance;
    controlPacket->readPrimitive(&rtt);
    controlPacket->readPrimitive(&rttVariance);
}

void Connection::processACK2(std::unique_ptr<ControlPacket> controlPacket) {
    
}

void Connection::processNAK(std::unique_ptr<ControlPacket> controlPacket) {
    // read the loss report
    SeqNum start, end;
    controlPacket->readPrimitive(&start);
    if (controlPacket->bytesLeftToRead() >= (qint64)sizeof(SeqNum)) {
        controlPacket->readPrimitive(&end);
    }
}
