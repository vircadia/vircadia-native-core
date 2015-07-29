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
class Socket;
    
class SendQueue : public QObject {
    Q_OBJECT
    
public:
    static const int DEFAULT_SEND_PERIOD = 16 * 1000; // 16ms, in microseconds
    
    static std::unique_ptr<SendQueue> create(Socket* socket, HifiSockAddr dest);
    
    void queuePacket(std::unique_ptr<Packet> packet);
    int getQueueSize() const { QReadLocker locker(&_packetsLock); return _packets.size(); }

    SequenceNumber getCurrentSequenceNumber() const { return SequenceNumber(_atomicCurrentSequenceNumber); }
    
    void setFlowWindowSize(int flowWindowSize) { _flowWindowSize = flowWindowSize; }
    
    int getPacketSendPeriod() const { return _packetSendPeriod; }
    void setPacketSendPeriod(int newPeriod) { _packetSendPeriod = newPeriod; }
    
    // Send a packet through the socket
    void sendPacket(const BasePacket& packet);
    
public slots:
    void run();
    void stop();
    
    void ack(SequenceNumber ack);
    void nak(SequenceNumber start, SequenceNumber end);
    void overrideNAKListFromPacket(ControlPacket& packet);
    
private slots:
    void loop();
    
private:
    SendQueue(Socket* socket, HifiSockAddr dest);
    SendQueue(SendQueue& other) = delete;
    SendQueue(SendQueue&& other) = delete;
    
    // Increments current sequence number and return it
    SequenceNumber getNextSequenceNumber();
    
    mutable QReadWriteLock _packetsLock; // Protects the packets to be sent list.
    std::list<std::unique_ptr<Packet>> _packets; // List of packets to be sent
    
    Socket* _socket { nullptr }; // Socket to send packet on
    HifiSockAddr _destination; // Destination addr
    
    std::atomic<uint32_t> _lastACKSequenceNumber; // Last ACKed sequence number
    
    SequenceNumber _currentSequenceNumber; // Last sequence number sent out
    std::atomic<uint32_t> _atomicCurrentSequenceNumber; // Atomic for last sequence number sent out
    
    std::atomic<int> _packetSendPeriod { 0 }; // Interval between two packet send event in microseconds
    std::chrono::high_resolution_clock::time_point _lastSendTimestamp; // Record last time of packet departure
    std::atomic<bool> _isRunning { false };
    
    std::atomic<int> _flowWindowSize { udt::MAX_PACKETS_IN_FLIGHT }; // Flow control window size (number of packets that can be on wire)
    
    mutable QReadWriteLock _naksLock; // Protects the naks list.
    LossList _naks; // Sequence numbers of packets to resend
    
    mutable QReadWriteLock _sentLock; // Protects the sent packet list
    std::unordered_map<SequenceNumber, std::unique_ptr<Packet>> _sentPackets; // Packets waiting for ACK.
};
    
}
    
#endif // hifi_SendQueue_h
