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

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>

#include <PortableHighResolutionClock.h>

#include "../HifiSockAddr.h"

#include "Constants.h"
#include "PacketQueue.h"
#include "SequenceNumber.h"
#include "LossList.h"

namespace udt {
    
class BasePacket;
class ControlPacket;
class Packet;
class PacketList;
class Socket;
    
class SendQueue : public QObject {
    Q_OBJECT
    
public:
    enum class State {
        NotStarted,
        Running,
        Stopped
    };
    
    static std::unique_ptr<SendQueue> create(Socket* socket, HifiSockAddr destination);
    
    void queuePacket(std::unique_ptr<Packet> packet);
    void queuePacketList(std::unique_ptr<PacketList> packetList);

    SequenceNumber getCurrentSequenceNumber() const { return SequenceNumber(_atomicCurrentSequenceNumber); }
    
    void setFlowWindowSize(int flowWindowSize) { _flowWindowSize = flowWindowSize; }
    
    int getPacketSendPeriod() const { return _packetSendPeriod; }
    void setPacketSendPeriod(int newPeriod) { _packetSendPeriod = newPeriod; }
    
    void setEstimatedTimeout(int estimatedTimeout) { _estimatedTimeout = estimatedTimeout; }
    void setSyncInterval(int syncInterval) { _syncInterval = syncInterval; }

    void setProbePacketEnabled(bool enabled);
    
public slots:
    void stop();
    
    void ack(SequenceNumber ack);
    void nak(SequenceNumber start, SequenceNumber end);
    void fastRetransmit(SequenceNumber ack);
    void overrideNAKListFromPacket(ControlPacket& packet);
    void handshakeACK(SequenceNumber initialSequenceNumber);

signals:
    void packetSent(int dataSize, int payloadSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint);
    void packetRetransmitted(int dataSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint);
    
    void queueInactive();

    void shortCircuitLoss(quint32 sequenceNumber);
    void timeout();
    
private slots:
    void run();
    
private:
    SendQueue(Socket* socket, HifiSockAddr dest);
    SendQueue(SendQueue& other) = delete;
    SendQueue(SendQueue&& other) = delete;
    
    void sendHandshake();
    
    int sendPacket(const Packet& packet);
    bool sendNewPacketAndAddToSentList(std::unique_ptr<Packet> newPacket, SequenceNumber sequenceNumber);
    
    int maybeSendNewPacket(); // Figures out what packet to send next
    bool maybeResendPacket(); // Determines whether to resend a packet and which one
    
    bool isInactive(bool attemptedToSendPacket);
    void deactivate(); // makes the queue inactive and cleans it up

    bool isFlowWindowFull() const;
    
    // Increments current sequence number and return it
    SequenceNumber getNextSequenceNumber();
    
    PacketQueue _packets;
    
    Socket* _socket { nullptr }; // Socket to send packet on
    HifiSockAddr _destination; // Destination addr

    SequenceNumber _initialSequenceNumber; // Randomized on SendQueue creation, identifies connection during re-connect requests
    
    std::atomic<uint32_t> _lastACKSequenceNumber { 0 }; // Last ACKed sequence number
    
    SequenceNumber _currentSequenceNumber { 0 }; // Last sequence number sent out
    std::atomic<uint32_t> _atomicCurrentSequenceNumber { 0 }; // Atomic for last sequence number sent out
    
    std::atomic<int> _packetSendPeriod { 0 }; // Interval between two packet send event in microseconds, set from CC
    std::atomic<State> _state { State::NotStarted };
    
    std::atomic<int> _estimatedTimeout { 0 }; // Estimated timeout, set from CC
    std::atomic<int> _syncInterval { udt::DEFAULT_SYN_INTERVAL_USECS }; // Sync interval, set from CC
    std::atomic<int64_t> _lastReceiverResponse { 0 }; // Timestamp for the last time we got new data from the receiver (ACK/NAK)
    
    std::atomic<int> _flowWindowSize { 0 }; // Flow control window size (number of packets that can be on wire) - set from CC
    
    mutable std::mutex _naksLock; // Protects the naks list.
    LossList _naks; // Sequence numbers of packets to resend
    
    mutable QReadWriteLock _sentLock; // Protects the sent packet list
    using PacketResendPair = std::pair<uint8_t, std::unique_ptr<Packet>>; // Number of resend + packet ptr
    std::unordered_map<SequenceNumber, PacketResendPair> _sentPackets; // Packets waiting for ACK.
    
    std::mutex _handshakeMutex; // Protects the handshake ACK condition_variable
    std::atomic<bool> _hasReceivedHandshakeACK { false }; // flag for receipt of handshake ACK from client
    std::condition_variable _handshakeACKCondition;
    
    std::condition_variable_any _emptyCondition;


    std::atomic<bool> _shouldSendProbes { true };
};
    
}
    
#endif // hifi_SendQueue_h
