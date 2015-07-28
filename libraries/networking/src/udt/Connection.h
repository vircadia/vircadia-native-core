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

#include "LossList.h"
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
    
    SeqNum getLastReceivedACK() const { return SeqNum(_atomicLastReceivedACK); }
    
    void processReceivedSeqNum(SeqNum seq);
    void processControl(std::unique_ptr<ControlPacket> controlPacket);
    
private:
    void processACK(std::unique_ptr<ControlPacket> controlPacket);
    void processLightACK(std::unique_ptr<ControlPacket> controlPacket);
    void processACK2(std::unique_ptr<ControlPacket> controlPacket);
    void processNAK(std::unique_ptr<ControlPacket> controlPacket);
    
    void updateRTT(int32_t rtt);
    
    int _synInterval; // Periodical Rate Control Interval, defaults to 10ms
    
    LossList _lossList; // List of all missing packets
    SeqNum _largestReceivedSeqNum; // The largest sequence number received from the peer
    SeqNum _lastReceivedACK; // The last ACK received
    std::atomic<uint32_t> _atomicLastReceivedACK; // Atomic for thread-safe get of last ACK received
    SeqNum _lastReceivedAcknowledgedACK; // The last sent ACK that has been acknowledged via an ACK2 from the peer
    SeqNum _currentACKSubSequenceNumber; // The current ACK sub-sequence number (used for Acknowledgment of ACKs)
    
    SeqNum _lastSentACK; // The last sent ACK
    SeqNum _lastSentACK2; // The last sent ACK sub-sequence number in an ACK2
    
    int _totalReceivedACKs { 0 };
    
    int32_t _rtt; // RTT, in milliseconds
    int32_t _rttVariance; // RTT variance
    int _flowWindowSize; // Flow control window size
    
    std::unique_ptr<SendQueue> _sendQueue;
};
    
}

#endif // hifi_Connection_h
