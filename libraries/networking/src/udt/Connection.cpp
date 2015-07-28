//
//  Connection.cpp
//  libraries/networking/src/udt
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
        + sizeof(_rtt) + sizeof(int32_t) + sizeof(int32_t);
    
    // setup the ACK packet, make it static so we can re-use it
    static auto ackPacket = ControlPacket::create(ControlPacket::ACK, ACK_PACKET_PAYLOAD_BYTES);
    ackPacket->reset(); // We need to reset it every time.
    
    auto currentTime = high_resolution_clock::now();
    static high_resolution_clock::time_point lastACKSendTime;
    
    SequenceNumber nextACKNumber = nextACK();
    
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
        milliseconds sinceLastACK = duration_cast<milliseconds>(currentTime - lastACKSendTime);
        
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
    
    // pack the available buffer size - must be a minimum of 2
    
    if (wasCausedBySyncTimeout) {
        // pack in the receive speed and estimatedBandwidth
        ackPacket->writePrimitive(_receiveWindow.getPacketReceiveSpeed());
        ackPacket->writePrimitive(_receiveWindow.getEstimatedBandwidth());
        
        // record this as the last ACK send time
        lastACKSendTime = high_resolution_clock::now();
    }
    
    // have the send queue send off our packet
    _sendQueue->sendPacket(*ackPacket);
    
    // write this ACK to the map of sent ACKs
    _sentACKs[_currentACKSubSequenceNumber] = { nextACKNumber, high_resolution_clock::now() };
}

void Connection::sendLightACK() const {
    // create the light ACK packet, make it static so we can re-use it
    static const int LIGHT_ACK_PACKET_PAYLOAD_BYTES = sizeof(SequenceNumber);
    static auto lightACKPacket = ControlPacket::create(ControlPacket::ACK, LIGHT_ACK_PACKET_PAYLOAD_BYTES);
    
    SequenceNumber nextACKNumber = nextACK();
    
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

SequenceNumber Connection::nextACK() const {
    if (_lossList.getLength() > 0) {
        return _lossList.getFirstSequenceNumber();
    } else {
        return _lastReceivedSequenceNumber;
    }
}

void Connection::processReceivedSequenceNumber(SequenceNumber seq) {
    
    // check if this is a packet pair we should estimate bandwidth from, or just a regular packet
    if (((uint32_t) seq & 0xF) == 0) {
        _receiveWindow.onProbePair1Arrival();
    } else if (((uint32_t) seq & 0xF) == 1) {
        _receiveWindow.onProbePair2Arrival();
    } else {
        _receiveWindow.onPacketArrival();
    }
    
    // If this is not the next sequence number, report loss
    if (seq > _lastReceivedSequenceNumber + 1) {
        if (_lastReceivedSequenceNumber + 1 == seq - 1) {
            _lossList.append(_lastReceivedSequenceNumber + 1);
        } else {
            _lossList.append(_lastReceivedSequenceNumber + 1, seq - 1);
        }
        
        // create the loss report packet, make it static so we can re-use it
        static const int NAK_PACKET_PAYLOAD_BYTES = 2 * sizeof(SequenceNumber);
        static auto lossReport = ControlPacket::create(ControlPacket::NAK, NAK_PACKET_PAYLOAD_BYTES);
        lossReport->reset(); // We need to reset it every time.
        
        // pack in the loss report
        lossReport->writePrimitive(_lastReceivedSequenceNumber + 1);
        if (_lastReceivedSequenceNumber + 1 != seq - 1) {
            lossReport->writePrimitive(seq - 1);
        }
        
        // have the send queue send off our packet immediately
        _sendQueue->sendPacket(*lossReport);
    }
    
    if (seq > _lastReceivedSequenceNumber) {
        // Update largest recieved sequence number
        _lastReceivedSequenceNumber = seq;
    } else {
        // Otherwise, it's a resend, remove it from the loss list
        _lossList.remove(seq);
    }
}

void Connection::processControl(unique_ptr<ControlPacket> controlPacket) {
    switch (controlPacket->getType()) {
        case ControlPacket::ACK: {
            if (controlPacket->getPayloadSize() == sizeof(SequenceNumber)) {
                processLightACK(move(controlPacket));
            } else {
                processACK(move(controlPacket));
            }
            break;
        }
        case ControlPacket::ACK2:
            processACK2(move(controlPacket));
            break;
        case ControlPacket::NAK:
            processNAK(move(controlPacket));
            break;
    }
}

void Connection::processACK(std::unique_ptr<ControlPacket> controlPacket) {
    // read the ACK sub-sequence number
    SequenceNumber currentACKSubSequenceNumber;
    controlPacket->readPrimitive(&currentACKSubSequenceNumber);
    
    // Check if we need send an ACK2 for this ACK
    // This will be the case if it has been longer than the sync interval OR
    // it looks like they haven't received our ACK2 for this ACK
    auto currentTime = high_resolution_clock::now();
    static high_resolution_clock::time_point lastACK2SendTime;
    
    milliseconds sinceLastACK2 = duration_cast<milliseconds>(currentTime - lastACK2SendTime);
    
    if (sinceLastACK2.count() > _synInterval || currentACKSubSequenceNumber == _lastSentACK2) {
        // setup a static ACK2 packet we will re-use
        static const int ACK2_PAYLOAD_BYTES = sizeof(SequenceNumber);
        static auto ack2Packet = ControlPacket::create(ControlPacket::ACK2, ACK2_PAYLOAD_BYTES);
        
        // reset the ACK2 Packet before writing the sub-sequence number to it
        ack2Packet->reset();
        
        // write the sub sequence number for this ACK2
        ack2Packet->writePrimitive(currentACKSubSequenceNumber);
        
        // update the last sent ACK2 and the last ACK2 send time
        _lastSentACK2 = currentACKSubSequenceNumber;
        lastACK2SendTime = high_resolution_clock::now();
    }
    
    // read the ACKed sequence number
    SequenceNumber ack;
    controlPacket->readPrimitive(&ack);
    
    // validate that this isn't a BS ACK
    if (ack > _sendQueue->getCurrentSequenceNumber()) {
        // in UDT they specifically break the connection here - do we want to do anything?
        return;
    }
    
    // read the RTT
    int32_t rtt;
    controlPacket->readPrimitive(&rtt);
    
    // read the desired flow window size
    int flowWindowSize;
    controlPacket->readPrimitive(&flowWindowSize);
    
    if (ack <= _lastReceivedACK) {
        // this is a valid ACKed sequence number - update the flow window size and the last received ACK
        _flowWindowSize = flowWindowSize;
        _lastReceivedACK = ack;
    }
    
    // make sure this isn't a repeated ACK
    if (ack <= SequenceNumber(_atomicLastReceivedACK)) {
        return;
    }
    
    // ACK the send queue so it knows what was received
    _sendQueue->ack(ack);
    
    // update the atomic for last received ACK, the send queue uses this to re-transmit
    _atomicLastReceivedACK.store((uint32_t) _lastReceivedACK);
    
    // remove everything up to this ACK from the sender loss list
    
    // update the RTT
    updateRTT(rtt);
    
    // set the RTT for congestion control
    
    if (controlPacket->getPayloadSize() > (qint64) (sizeof(SequenceNumber) + sizeof(SequenceNumber) + sizeof(rtt))) {
        int32_t deliveryRate, bandwidth;
        controlPacket->readPrimitive(&deliveryRate);
        controlPacket->readPrimitive(&bandwidth);
        
        // set the delivery rate and bandwidth for congestion control
    }
    
    // fire the onACK callback for congestion control
    
    // update the total count of received ACKs
    ++_totalReceivedACKs;
}

void Connection::processLightACK(std::unique_ptr<ControlPacket> controlPacket) {
    // read the ACKed sequence number
    SequenceNumber ack;
    controlPacket->readPrimitive(&ack);
    
    // must be larger than the last received ACK to be processed
    if (ack > _lastReceivedACK) {
        // decrease the flow window size by the offset between the last received ACK and this ACK
        _flowWindowSize -= seqoff(_lastReceivedACK, ack);
    
        // update the last received ACK to the this one
        _lastReceivedACK = ack;
    }
}

void Connection::processACK2(std::unique_ptr<ControlPacket> controlPacket) {
    // pull the sub sequence number from the packet
    SequenceNumber subSequenceNumber;
    controlPacket->readPrimitive(&subSequenceNumber);

    // check if we had that subsequence number in our map
    auto it = _sentACKs.find(subSequenceNumber);
    if (it != _sentACKs.end()) {
        // update the RTT using the ACK window
        SequenceNumberTimePair& pair = it->second;
        
        // calculate the RTT (time now - time ACK sent)
        auto now = high_resolution_clock::now();
        int rtt = duration_cast<milliseconds>(now - pair.second).count();
        
        updateRTT(rtt);
        
        // set the RTT for congestion control
        
        // update the last ACKed ACK
        if (pair.first > _lastReceivedAcknowledgedACK) {
            _lastReceivedAcknowledgedACK = pair.first;
        }
    }
}

void Connection::processNAK(std::unique_ptr<ControlPacket> controlPacket) {
    // read the loss report
    SequenceNumber start, end;
    controlPacket->readPrimitive(&start);
    if (controlPacket->bytesLeftToRead() >= (qint64)sizeof(SequenceNumber)) {
        controlPacket->readPrimitive(&end);
    }
}

void Connection::updateRTT(int rtt) {
    // This updates the RTT using exponential weighted moving average
    // This is the Jacobson's forumla for RTT estimation
    // http://www.mathcs.emory.edu/~cheung/Courses/455/Syllabus/7-transport/Jacobson-88.pdf
    
    // Estimated RTT = (1 - x)(estimatedRTT) + (x)(sampleRTT)
    // (where x = 0.125 via Jacobson)
    
    // Deviation  = (1 - x)(deviation) + x |sampleRTT - estimatedRTT|
    // (where x = 0.25 via Jacobson)
    
    _rttVariance = (_rttVariance * 3 + abs(rtt - _rtt)) >> 2;
    _rtt = (_rtt * 7 + rtt) >> 3;
}
