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

#include <list>
#include <memory>

#include <QtCore/QObject>

#include <PortableHighResolutionClock.h>

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
class PacketList;
class Socket;

class PendingReceivedMessage {
public:
    void enqueuePacket(std::unique_ptr<Packet> packet);
    bool isComplete() const { return _hasLastPacket && _numPackets == _packets.size(); }
    bool hasAvailablePackets() const;
    std::unique_ptr<Packet> removeNextPacket();
    
    std::list<std::unique_ptr<Packet>> _packets;

private:
    bool _hasLastPacket { false };
    Packet::MessagePartNumber _nextPartNumber = 0;
    unsigned int _numPackets { 0 };
};

class Connection : public QObject {
    Q_OBJECT
public:
    using SequenceNumberTimePair = std::pair<SequenceNumber, p_high_resolution_clock::time_point>;
    using ACKListPair = std::pair<SequenceNumber, SequenceNumberTimePair>;
    using SentACKList = std::list<ACKListPair>;
    using ControlPacketPointer = std::unique_ptr<ControlPacket>;
    
    Connection(Socket* parentSocket, HifiSockAddr destination, std::unique_ptr<CongestionControl> congestionControl);
    virtual ~Connection();

    void sendReliablePacket(std::unique_ptr<Packet> packet);
    void sendReliablePacketList(std::unique_ptr<PacketList> packet);

    void sync(); // rate control method, fired by Socket for all connections on SYN interval

    // return indicates if this packet should be processed
    bool processReceivedSequenceNumber(SequenceNumber sequenceNumber, int packetSize, int payloadSize);
    void processControl(ControlPacketPointer controlPacket);

    void queueReceivedMessagePacket(std::unique_ptr<Packet> packet);
    
    ConnectionStats::Stats sampleStats() { return _stats.sample(); }
    
    bool isActive() const { return _isActive; }

    HifiSockAddr getDestination() const { return _destination; }

    void setMaxBandwidth(int maxBandwidth);

    void sendHandshakeRequest();

signals:
    void packetSent();
    void connectionInactive(const HifiSockAddr& sockAddr);
    void receiverHandshakeRequestComplete(const HifiSockAddr& sockAddr);

private slots:
    void recordSentPackets(int wireSize, int payloadSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint);
    void recordRetransmission(int wireSize, SequenceNumber sequenceNumber, p_high_resolution_clock::time_point timePoint);
    void queueInactive();
    void queueTimeout();
    void queueShortCircuitLoss(quint32 sequenceNumber);
    
private:
    void sendACK(bool wasCausedBySyncTimeout = true);
    void sendLightACK();
    void sendACK2(SequenceNumber currentACKSubSequenceNumber);
    void sendNAK(SequenceNumber sequenceNumberRecieved);
    void sendTimeoutNAK();
    
    void processACK(ControlPacketPointer controlPacket);
    void processLightACK(ControlPacketPointer controlPacket);
    void processACK2(ControlPacketPointer controlPacket);
    void processNAK(ControlPacketPointer controlPacket);
    void processTimeoutNAK(ControlPacketPointer controlPacket);
    void processHandshake(ControlPacketPointer controlPacket);
    void processHandshakeACK(ControlPacketPointer controlPacket);
    void processProbeTail(ControlPacketPointer controlPacket);
    
    void resetReceiveState();
    void resetRTT();
    
    void deactivate() { _isActive = false; emit connectionInactive(_destination); }
    
    SendQueue& getSendQueue();
    SequenceNumber nextACK() const;
    void updateRTT(int rtt);
    
    int estimatedTimeout() const;
    
    void updateCongestionControlAndSendQueue(std::function<void()> congestionCallback);
    
    void stopSendQueue();
    
    int _synInterval; // Periodical Rate Control Interval, in microseconds
    
    int _nakInterval { -1 }; // NAK timeout interval, in microseconds, set on loss
    int _minNAKInterval { 100000 }; // NAK timeout interval lower bound, default of 100ms
    p_high_resolution_clock::time_point _lastNAKTime = p_high_resolution_clock::now();
    
    bool _hasReceivedHandshake { false }; // flag for receipt of handshake from server
    bool _hasReceivedHandshakeACK { false }; // flag for receipt of handshake ACK from client
    bool _didRequestHandshake { false }; // flag for request of handshake from server
   
    p_high_resolution_clock::time_point _connectionStart = p_high_resolution_clock::now(); // holds the time_point for creation of this connection
    p_high_resolution_clock::time_point _lastReceiveTime; // holds the last time we received anything from sender
    
    bool _isReceivingData { false }; // flag used for expiry of receipt portion of connection
    bool _isActive { true }; // flag used for inactivity of connection

    SequenceNumber _initialReceiveSequenceNumber; // Randomized by peer SendQueue on creation, identifies connection during re-connect requests

    LossList _lossList; // List of all missing packets
    SequenceNumber _lastReceivedSequenceNumber; // The largest sequence number received from the peer
    SequenceNumber _lastReceivedACK; // The last ACK received
    SequenceNumber _lastReceivedAcknowledgedACK; // The last sent ACK that has been acknowledged via an ACK2 from the peer
    SequenceNumber _currentACKSubSequenceNumber; // The current ACK sub-sequence number (used for Acknowledgment of ACKs)
    
    SequenceNumber _lastSentACK; // The last sent ACK
    SequenceNumber _lastSentACK2; // The last sent ACK sub-sequence number in an ACK2

    int _acksDuringSYN { 1 }; // The number of non-SYN ACKs sent during SYN
    int _lightACKsDuringSYN { 1 }; // The number of lite ACKs sent during SYN interval
    
    int32_t _rtt; // RTT, in microseconds
    int32_t _rttVariance; // RTT variance
    int _flowWindowSize { udt::MAX_PACKETS_IN_FLIGHT }; // Flow control window size
    
    int _bandwidth { 1 }; // Exponential moving average for estimated bandwidth, in packets per second
    int _deliveryRate { 16 }; // Exponential moving average for receiver's receive rate, in packets per second
    
    SentACKList _sentACKs; // Map of ACK sub-sequence numbers to ACKed sequence number and sent time
    
    Socket* _parentSocket { nullptr };
    HifiSockAddr _destination;
    
    PacketTimeWindow _receiveWindow { 16, 64 }; // Window of interval between packets (16) and probes (64) for timing
    bool _receivedControlProbeTail { false }; // Marker for receipt of control packet probe tail (in lieu of probe with data)
   
    std::unique_ptr<CongestionControl> _congestionControl;
   
    std::unique_ptr<SendQueue> _sendQueue;
    
    std::map<MessageNumber, PendingReceivedMessage> _pendingReceivedMessages;
    
    int _packetsSinceACK { 0 }; // The number of packets that have been received during the current ACK interval

    // Re-used control packets
    ControlPacketPointer _ackPacket;
    ControlPacketPointer _lightACKPacket;
    ControlPacketPointer _ack2Packet;
    ControlPacketPointer _lossReport;
    ControlPacketPointer _handshakeACK;

    ConnectionStats _stats;
};
    
}

#endif // hifi_Connection_h
