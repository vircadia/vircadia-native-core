//
//  Connection.h
//  libraries/networking/src/udt
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
#include "PacketTimeWindow.h"
#include "SendQueue.h"

class HifiSockAddr;

namespace udt {
    
class ControlPacket;
class Packet;
class Socket;

class Connection {

public:
    using SequenceNumberTimePair = std::pair<SequenceNumber, std::chrono::high_resolution_clock::time_point>;
    using SentACKMap = std::unordered_map<SequenceNumber, SequenceNumberTimePair>;
    
    Connection(Socket* parentSocket, HifiSockAddr destination);
    
    void send(std::unique_ptr<Packet> packet);
    
    void sendACK(bool wasCausedBySyncTimeout = true);
    void sendLightACK() const;
    
    SequenceNumber nextACK() const;
    
    SequenceNumber getLastReceivedACK() const { return SequenceNumber(_atomicLastReceivedACK); }
    
    void processReceivedSequenceNumber(SequenceNumber seq);
    void processControl(std::unique_ptr<ControlPacket> controlPacket);
    
private:
    void processACK(std::unique_ptr<ControlPacket> controlPacket);
    void processLightACK(std::unique_ptr<ControlPacket> controlPacket);
    void processACK2(std::unique_ptr<ControlPacket> controlPacket);
    void processNAK(std::unique_ptr<ControlPacket> controlPacket);
    
    void updateRTT(int rtt);
    
    int _synInterval; // Periodical Rate Control Interval, defaults to 10ms
    
    LossList _lossList; // List of all missing packets
    SequenceNumber _lastReceivedSequenceNumber { SequenceNumber::MAX }; // The largest sequence number received from the peer
    SequenceNumber _lastReceivedACK { SequenceNumber::MAX }; // The last ACK received
    std::atomic<uint32_t> _atomicLastReceivedACK { (uint32_t) SequenceNumber::MAX }; // Atomic for thread-safe get of last ACK received
    SequenceNumber _lastReceivedAcknowledgedACK { SequenceNumber::MAX }; // The last sent ACK that has been acknowledged via an ACK2 from the peer
    SequenceNumber _currentACKSubSequenceNumber; // The current ACK sub-sequence number (used for Acknowledgment of ACKs)
    
    SequenceNumber _lastSentACK { SequenceNumber::MAX }; // The last sent ACK
    SequenceNumber _lastSentACK2; // The last sent ACK sub-sequence number in an ACK2
    
    int _totalReceivedACKs { 0 };
    
    int32_t _rtt; // RTT, in milliseconds
    int32_t _rttVariance; // RTT variance
    int _flowWindowSize; // Flow control window size
    
    SentACKMap _sentACKs; // Map of ACK sub-sequence numbers to ACKed sequence number and sent time
    
    PacketTimeWindow _receiveWindow; // Window of received packets for bandwidth estimation and receive speed
    
    std::unique_ptr<SendQueue> _sendQueue;
};
    
}

#endif // hifi_Connection_h
