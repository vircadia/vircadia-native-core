//
//  SendQueue.h
//  libraries/networking/src/udt
//
//  Created by Clement on 7/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SendQueue_h
#define hifi_SendQueue_h

#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>
#include <unordered_map>

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>

#include "../HifiSockAddr.h"

#include "Constants.h"
#include "SequenceNumber.h"
#include "LossList.h"

namespace udt {
    
class BasePacket;
class ControlPacket;
class Packet;
class PacketList;
class Socket;

using MessageNumber = uint32_t;
    
class SendQueue : public QObject {
    Q_OBJECT
    
public:
    using high_resolution_clock = std::chrono::high_resolution_clock;
    using time_point = high_resolution_clock::time_point;
    
    static std::unique_ptr<SendQueue> create(Socket* socket, HifiSockAddr destination);
    
    void queuePacket(std::unique_ptr<Packet> packet);
    void queuePacketList(std::unique_ptr<PacketList> packetList);

    SequenceNumber getCurrentSequenceNumber() const { return SequenceNumber(_atomicCurrentSequenceNumber); }
    
    void setFlowWindowSize(int flowWindowSize) { _flowWindowSize = flowWindowSize; }
    
    int getPacketSendPeriod() const { return _packetSendPeriod; }
    void setPacketSendPeriod(int newPeriod) { _packetSendPeriod = newPeriod; }
    
    void setEstimatedTimeout(int estimatedTimeout) { _estimatedTimeout = estimatedTimeout; }
    
public slots:
    void stop();
    
    void ack(SequenceNumber ack);
    void nak(SequenceNumber start, SequenceNumber end);
    void overrideNAKListFromPacket(ControlPacket& packet);
    void handshakeACK();

signals:
    void packetSent(int dataSize, int payloadSize);
    void packetRetransmitted();
    
    void queueInactive();
    
private slots:
    void run();
    
private:
    SendQueue(Socket* socket, HifiSockAddr dest);
    SendQueue(SendQueue& other) = delete;
    SendQueue(SendQueue&& other) = delete;
    
    void sendPacket(const Packet& packet);
    void sendNewPacketAndAddToSentList(std::unique_ptr<Packet> newPacket, SequenceNumber sequenceNumber);
    
    bool maybeSendNewPacket(); // Figures out what packet to send next
    bool maybeResendPacket(); // Determines whether to resend a packet and which one
    
    void deactivate(); // makes the queue inactive and cleans it up
    
    // Increments current sequence number and return it
    SequenceNumber getNextSequenceNumber();
    MessageNumber getNextMessageNumber();
    
    mutable std::mutex _packetsLock; // Protects the packets to be sent list.
    std::list<std::unique_ptr<Packet>> _packets; // List of packets to be sent
    
    Socket* _socket { nullptr }; // Socket to send packet on
    HifiSockAddr _destination; // Destination addr
    
    std::atomic<uint32_t> _lastACKSequenceNumber { 0 }; // Last ACKed sequence number
    
    MessageNumber _currentMessageNumber { 0 };
    SequenceNumber _currentSequenceNumber; // Last sequence number sent out
    std::atomic<uint32_t> _atomicCurrentSequenceNumber { 0 };// Atomic for last sequence number sent out
    
    std::atomic<int> _packetSendPeriod { 0 }; // Interval between two packet send event in microseconds, set from CC
    std::atomic<bool> _isRunning { false };
    
    std::atomic<int> _estimatedTimeout { 0 }; // Estimated timeout, set from CC
    std::atomic<int> _timeoutExpiryCount { 0 }; // The number of times the timeout has expired without response from client
    std::atomic<uint64_t> _lastReceiverResponse { 0 }; // Timestamp for the last time we got new data from the receiver (ACK/NAK)
    
    std::atomic<int> _flowWindowSize { 0 }; // Flow control window size (number of packets that can be on wire) - set from CC
    
    mutable std::mutex _naksLock; // Protects the naks list.
    LossList _naks; // Sequence numbers of packets to resend
    
    mutable QReadWriteLock _sentLock; // Protects the sent packet list
    std::unordered_map<SequenceNumber, std::unique_ptr<Packet>> _sentPackets; // Packets waiting for ACK.
    
    std::mutex _handshakeMutex; // Protects the handshake ACK condition_variable
    std::atomic<bool> _hasReceivedHandshakeACK { false }; // flag for receipt of handshake ACK from client
    std::condition_variable _handshakeACKCondition;
    
    std::condition_variable_any _emptyCondition;
};
    
}
    
#endif // hifi_SendQueue_h
