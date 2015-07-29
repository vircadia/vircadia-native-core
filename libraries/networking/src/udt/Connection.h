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

#include "ConnectionStats.h"
#include "Constants.h"
#include "LossList.h"
#include "PacketTimeWindow.h"
#include "SendQueue.h"
#include "../HifiSockAddr.h"

namespace udt {
    
class CongestionControl;
class ControlPacket;
class Packet;
class Socket;

class Connection {

public:
    using SequenceNumberTimePair = std::pair<SequenceNumber, std::chrono::high_resolution_clock::time_point>;
    using SentACKMap = std::unordered_map<SequenceNumber, SequenceNumberTimePair>;
    
    Connection(Socket* parentSocket, HifiSockAddr destination, std::unique_ptr<CongestionControl> congestionControl);
    ~Connection();
    
    void sendReliablePacket(std::unique_ptr<Packet> packet);

    void sync(); // rate control method, fired by Socket for all connections on SYN interval
    
    void processReceivedSequenceNumber(SequenceNumber seq);
    void processControl(std::unique_ptr<ControlPacket> controlPacket);
    
private:
    void sendACK(bool wasCausedBySyncTimeout = true);
    void sendLightACK();
    void sendACK2(SequenceNumber currentACKSubSequenceNumber);
    void sendNAK(SequenceNumber sequenceNumberRecieved);
    void sendTimeoutNAK();
    
    void processACK(std::unique_ptr<ControlPacket> controlPacket);
    void processLightACK(std::unique_ptr<ControlPacket> controlPacket);
    void processACK2(std::unique_ptr<ControlPacket> controlPacket);
    void processNAK(std::unique_ptr<ControlPacket> controlPacket);
    void processTimeoutNAK(std::unique_ptr<ControlPacket> controlPacket);
    
    SequenceNumber nextACK() const;
    void updateRTT(int rtt);
    
    int estimatedTimeout() const;
    
    void updateCongestionControlAndSendQueue(std::function<void()> congestionCallback);
    
    int _synInterval; // Periodical Rate Control Interval, in microseconds
    
    int _nakInterval; // NAK timeout interval, in microseconds
    int _minNAKInterval { 100000 }; // NAK timeout interval lower bound, default of 100ms
    std::chrono::high_resolution_clock::time_point _lastNAKTime;
    
    LossList _lossList; // List of all missing packets
    SequenceNumber _lastReceivedSequenceNumber { SequenceNumber::MAX }; // The largest sequence number received from the peer
    SequenceNumber _lastReceivedACK { SequenceNumber::MAX }; // The last ACK received
    SequenceNumber _lastReceivedAcknowledgedACK { SequenceNumber::MAX }; // The last sent ACK that has been acknowledged via an ACK2 from the peer
    SequenceNumber _currentACKSubSequenceNumber; // The current ACK sub-sequence number (used for Acknowledgment of ACKs)
    
    SequenceNumber _lastSentACK { SequenceNumber::MAX }; // The last sent ACK
    SequenceNumber _lastSentACK2; // The last sent ACK sub-sequence number in an ACK2
    
    int32_t _rtt; // RTT, in microseconds
    int32_t _rttVariance; // RTT variance
    int _flowWindowSize { udt::MAX_PACKETS_IN_FLIGHT }; // Flow control window size
    
    int _bandwidth { 1 }; // Exponential moving average for estimated bandwidth, in packets per second
    int _deliveryRate { 16 }; // Exponential moving average for receiver's receive rate, in packets per second
    
    SentACKMap _sentACKs; // Map of ACK sub-sequence numbers to ACKed sequence number and sent time
    
    Socket* _parentSocket { nullptr };
    HifiSockAddr _destination;
    
    PacketTimeWindow _receiveWindow { 16, 64 }; // Window of interval between packets (16) and probes (64) for bandwidth and receive speed
    
    std::unique_ptr<SendQueue> _sendQueue;
    
    std::unique_ptr<CongestionControl> _congestionControl;
    
    // Data packet stat collection
    int _totalReceivedDataPackets { 0 };
    int _packetsSinceACK { 0 }; // The number of packets that have been received during the current ACK interval
    
    ConnectionStats _stats;
};
    
}

#endif // hifi_Connection_h
