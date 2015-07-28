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

#include <list>
#include <unordered_map>

#include <QtCore/QObject>
#include <QtCore/QReadWriteLock>
#include <QtCore/QTimer>

#include "../HifiSockAddr.h"

#include "SequenceNumber.h"

namespace udt {
    
class Socket;
class BasePacket;
class Packet;
    
class SendQueue : public QObject {
    Q_OBJECT
    
public:
    static const int DEFAULT_SEND_PERIOD = 16; // msec
    
    static std::unique_ptr<SendQueue> create(Socket* socket, HifiSockAddr dest);
    
    void queuePacket(std::unique_ptr<Packet> packet);
    int getQueueSize() const { QReadLocker locker(&_packetsLock); return _packets.size(); }

    quint64 getLastSendTimestamp() const { return _lastSendTimestamp; }
    
    SequenceNumber getCurrentSequenceNumber() const { return SequenceNumber(_atomicCurrentSequenceNumber); }
    
    int getPacketSendPeriod() const { return _packetSendPeriod; }
    void setPacketSendPeriod(int newPeriod) { _packetSendPeriod = newPeriod; }
    
public slots:
    void start();
    void stop();
    
    void ack(SequenceNumber ack);
    void nak(std::list<SequenceNumber> naks);
    
private slots:
    void sendNextPacket();
    
private:
    friend struct std::default_delete<SendQueue>;
    
    SendQueue(Socket* socket, HifiSockAddr dest);
    SendQueue(SendQueue& other) = delete;
    SendQueue(SendQueue&& other) = delete;
    ~SendQueue();
    
    // Increments current sequence number and return it
    SequenceNumber getNextSequenceNumber();

    // Send a packet through the socket
    void sendPacket(const Packet& packet);
    
    mutable QReadWriteLock _packetsLock; // Protects the packets to be sent list.
    std::list<std::unique_ptr<Packet>> _packets; // List of packets to be sent
    std::unique_ptr<Packet> _nextPacket;  // Next packet to be sent
    
    Socket* _socket { nullptr }; // Socket to send packet on
    HifiSockAddr _destination; // Destination addr
    SequenceNumber _lastAck; // Last ACKed sequence number
    
    SequenceNumber _currentSequenceNumber; // Last sequence number sent out
    std::atomic<uint32_t> _atomicCurrentSequenceNumber; // Atomic for last sequence number sent out
    
    std::unique_ptr<QTimer> _sendTimer; // Send timer
    std::atomic<int> _packetSendPeriod { 0 }; // Interval between two packet send envent in msec
    std::atomic<quint64> _lastSendTimestamp { 0 }; // Record last time of packet departure
    std::atomic<bool> _running { false };
    
    mutable QReadWriteLock _naksLock; // Protects the naks list.
    std::list<SequenceNumber> _naks; // Sequence numbers of packets to resend
    
    mutable QReadWriteLock _sentLock; // Protects the sent packet list
    std::unordered_map<SequenceNumber, std::unique_ptr<Packet>> _sentPackets; // Packets waiting for ACK.
};
    
}
    
#endif // hifi_SendQueue_h
