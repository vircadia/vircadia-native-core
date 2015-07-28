//
//  Connection.h
//
//
//  Created by Clement on 7/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Connection_h
#define hifi_Connection_h

#include <chrono>
#include <memory>

#include "SendQueue.h"

class HifiSockAddr;

namespace udt {
    
class ControlPacket;
class Packet;
class Socket;

class Connection {

public:
    Connection(Socket* parentSocket, HifiSockAddr destination);
    
    void send(std::unique_ptr<Packet> packet);
    
    void sendACK(bool wasCausedBySyncTimeout = true);
    void sendLightACK() const;
    
    SeqNum nextACK() const;
    
    void processReceivedSeqNum(SeqNum seq);
    void processControl(std::unique_ptr<ControlPacket> controlPacket);
    
private:
    SeqNum _largestReceivedSeqNum; // The largest sequence number received from the peer
    SeqNum _lastSentACK; // The last sent ACK
    SeqNum _lastReceivedAcknowledgedACK; // The last sent ACK that has been acknowledged via an ACK2 from the peer
    SeqNum _currentACKSubSequenceNumber; // The current ACK sub-sequence number (used for Acknowledgment of ACKs)
    
    int32_t _rtt; // RTT, in milliseconds
    int32_t _rttVariance; // RTT variance
    
    std::chrono::high_resolution_clock::time_point _lastACKTime;
    
    std::unique_ptr<SendQueue> _sendQueue;
};
    
}

#endif // hifi_Connection_h
