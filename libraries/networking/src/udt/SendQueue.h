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
#include <list>
#include <unordered_map>

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtCore/QTimer>

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
    static std::unique_ptr<SendQueue> create(Socket* socket, HifiSockAddr destination);
    
    void queuePacket(std::unique_ptr<Packet> packet);
    void queuePacketList(std::unique_ptr<PacketList> packetList);
    int getQueueSize() const { QReadLocker locker(&_packetsLock); return _packets.size(); }

    SequenceNumber getCurrentSequenceNumber() const { return SequenceNumber(_atomicCurrentSequenceNumber); }
    
    void setFlowWindowSize(int flowWindowSize) { _flowWindowSize = flowWindowSize; }
    
    int getPacketSendPeriod() const { return _packetSendPeriod; }
    void setPacketSendPeriod(int newPeriod) { _packetSendPeriod = newPeriod; }
    
public slots:
    void stop();
    
    void ack(SequenceNumber ack);
    void nak(SequenceNumber start, SequenceNumber end);
    void overrideNAKListFromPacket(ControlPacket& packet);

signals:
    void packetSent(int dataSize, int payloadSize);
    void packetRetransmitted();
    
private slots:
    void run();
    
private:
    SendQueue(Socket* socket, HifiSockAddr dest);
    SendQueue(SendQueue& other) = delete;
    SendQueue(SendQueue&& other) = delete;
    
    void sendPacket(const Packet& packet);
    void sendNewPacketAndAddToSentList(std::unique_ptr<Packet> newPacket, SequenceNumber sequenceNumber);
    
    // Increments current sequence number and return it
    SequenceNumber getNextSequenceNumber();
    MessageNumber getNextMessageNumber();
    
    mutable QReadWriteLock _packetsLock; // Protects the packets to be sent list.
    std::list<std::unique_ptr<Packet>> _packets; // List of packets to be sent
    
    Socket* _socket { nullptr }; // Socket to send packet on
    HifiSockAddr _destination; // Destination addr
    
    std::atomic<uint32_t> _lastACKSequenceNumber; // Last ACKed sequence number
    
    MessageNumber _currentMessageNumber { 0 };
    SequenceNumber _currentSequenceNumber; // Last sequence number sent out
    std::atomic<uint32_t> _atomicCurrentSequenceNumber;// Atomic for last sequence number sent out
    
    std::atomic<int> _packetSendPeriod; // Interval between two packet send event in microseconds, set from CC
    std::chrono::high_resolution_clock::time_point _lastSendTimestamp; // Record last time of packet departure
    std::atomic<bool> _isRunning { false };
    
    std::atomic<int> _flowWindowSize; // Flow control window size (number of packets that can be on wire) - set from CC
    
    mutable QReadWriteLock _naksLock; // Protects the naks list.
    LossList _naks; // Sequence numbers of packets to resend
    
    mutable QReadWriteLock _sentLock; // Protects the sent packet list
    std::unordered_map<SequenceNumber, std::unique_ptr<Packet>> _sentPackets; // Packets waiting for ACK.
};
    
}
    
#endif // hifi_SendQueue_h
