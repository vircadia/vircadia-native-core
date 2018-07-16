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

    HifiSockAddr getDestination() const { return _destination; }

    void setMaxBandwidth(int maxBandwidth);

    void sendHandshakeRequest();

signals:
    void packetSent();
    void receiverHandshakeRequestComplete(const HifiSockAddr& sockAddr);

private slots:
    void recordSentPackets(int wireSize, int payloadSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint);
    void recordRetransmission(int wireSize, SequenceNumber sequenceNumber, p_high_resolution_clock::time_point timePoint);
    void queueInactive();
    void queueTimeout();
    
private:
    void sendACK(bool wasCausedBySyncTimeout = true);
    
    void processACK(ControlPacketPointer controlPacket);
    void processHandshake(ControlPacketPointer controlPacket);
    void processHandshakeACK(ControlPacketPointer controlPacket);
    
    void resetReceiveState();
    void resetRTT();
    
    SendQueue& getSendQueue();
    SequenceNumber nextACK() const;
    void updateRTT(int rtt);
    
    int estimatedTimeout() const;
    
    void updateCongestionControlAndSendQueue(std::function<void()> congestionCallback);
    
    void stopSendQueue();
    
    int _synInterval; // Periodical Rate Control Interval, in microseconds
    
    bool _hasReceivedHandshake { false }; // flag for receipt of handshake from server
    bool _hasReceivedHandshakeACK { false }; // flag for receipt of handshake ACK from client
    bool _didRequestHandshake { false }; // flag for request of handshake from server
   
    p_high_resolution_clock::time_point _connectionStart = p_high_resolution_clock::now(); // holds the time_point for creation of this connection
    p_high_resolution_clock::time_point _lastReceiveTime; // holds the last time we received anything from sender
    
    bool _isReceivingData { false }; // flag used for expiry of receipt portion of connection

    SequenceNumber _initialSequenceNumber; // Randomized on Connection creation, identifies connection during re-connect requests
    SequenceNumber _initialReceiveSequenceNumber; // Randomized by peer Connection on creation, identifies connection during re-connect requests

    MessageNumber _lastMessageNumber { 0 };

    LossList _lossList; // List of all missing packets
    SequenceNumber _lastReceivedSequenceNumber; // The largest sequence number received from the peer
    SequenceNumber _lastReceivedACK; // The last ACK received
    
    SequenceNumber _lastSentACK; // The last sent ACK
    
    int32_t _rtt; // RTT, in microseconds
    int32_t _rttVariance; // RTT variance
    int _flowWindowSize { udt::MAX_PACKETS_IN_FLIGHT }; // Flow control window size
    
    int _deliveryRate { 16 }; // Exponential moving average for receiver's receive rate, in packets per second
    
    Socket* _parentSocket { nullptr };
    HifiSockAddr _destination;
    
    PacketTimeWindow _receiveWindow { 16 }; // Window of interval between packets (16)
   
    std::unique_ptr<CongestionControl> _congestionControl;
   
    std::unique_ptr<SendQueue> _sendQueue;
    
    std::map<MessageNumber, PendingReceivedMessage> _pendingReceivedMessages;

    // Re-used control packets
    ControlPacketPointer _ackPacket;
    ControlPacketPointer _handshakeACK;

    ConnectionStats _stats;
};
    
}

#endif // hifi_Connection_h
