//
//  SendQueue.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 7/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SendQueue.h"

#include <algorithm>

#include <QtCore/QThread>

#include <SharedUtil.h>

#include "Packet.h"
#include "Socket.h"

using namespace udt;

std::unique_ptr<SendQueue> SendQueue::create(Socket* socket, HifiSockAddr dest) {
    auto queue = std::unique_ptr<SendQueue>(new SendQueue(socket, dest));
    
    // Setup queue private thread
    QThread* thread = new QThread(queue.get());
    thread->setObjectName("Networking: SendQueue"); // Name thread for easier debug
    connect(queue.get(), &QObject::destroyed, thread, &QThread::quit); // Thread auto cleanup
    connect(thread, &QThread::finished, thread, &QThread::deleteLater); // Thread auto cleanup
    
    // Move queue to private thread and start it
    queue->moveToThread(thread);
    thread->start();
    
    return std::move(queue);
}
    
SendQueue::SendQueue(Socket* socket, HifiSockAddr dest) :
    _socket(socket),
    _destination(dest)
{
    _sendTimer.reset(new QTimer(this));
    _sendTimer->setSingleShot(true);
    QObject::connect(_sendTimer.get(), &QTimer::timeout, this, &SendQueue::sendNextPacket);
    
    _packetSendPeriod = DEFAULT_SEND_PERIOD;
    _lastSendTimestamp = 0;
}

SendQueue::~SendQueue() {
    assert(thread() == QThread::currentThread());
    _sendTimer->stop();
}

void SendQueue::queuePacket(std::unique_ptr<Packet> packet) {
    {
        QWriteLocker locker(&_packetsLock);
        _packets.push_back(std::move(packet));
    }
    if (!_running) {
        start();
    }
}

void SendQueue::start() {
    // We need to make sure this is called on the right thread
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
    }
    _running = true;
    
    // This will send a packet and fire the send timer
    sendNextPacket();
}

void SendQueue::stop() {
    _running = false;
}
    
void SendQueue::sendPacket(const BasePacket& packet) {
    if (_socket) {
        _socket->writeDatagram(packet.getData(), packet.getDataSize(), _destination);
    }
}
    
void SendQueue::ack(SequenceNumber ack) {
    if (_lastAck == ack) {
        return;
    }
    
    {
        // remove any ACKed packets from the map of sent packets
        QWriteLocker locker(&_sentLock);
        for (auto seq = _lastAck; seq <= ack; ++seq) {
            _sentPackets.erase(seq);
        }
    }
    
    {
        // remove any sequence numbers equal to or lower than this ACK in the loss list
        QWriteLocker nakLocker(&_naksLock);
        
        auto it = _naks.begin();
        
        while (it != _naks.end()) {
            if (*it <= ack) {
                it = _naks.erase(it);
            } else {
                // the NAKs in the NAK list must be in order, so we can break if we hit one > ack
                break;
            }
        }
    }
    
    _lastAck = ack;
}

void SendQueue::nak(std::list<SequenceNumber> naks) {
    QWriteLocker locker(&_naksLock);
    _naks.splice(_naks.end(), naks); // Add naks at the end
}

SequenceNumber SendQueue::getNextSequenceNumber() {
    _atomicCurrentSequenceNumber = (SequenceNumber::Type)++_currentSequenceNumber;
    return _currentSequenceNumber;
}

void SendQueue::sendNextPacket() {
    if (!_running) {
        return;
    }
    
    // Record timing
    auto sendTime = msecTimestampNow(); // msec
    _lastSendTimestamp = sendTime;
    
    if (_nextPacket) {
        // Write packet's sequence number and send it off
        _nextPacket->writeSequenceNumber(getNextSequenceNumber());
        sendPacket(*_nextPacket);
        
        // Insert the packet we have just sent in the sent list
        QWriteLocker locker(&_sentLock);
        _sentPackets[_nextPacket->getSequenceNumber()].swap(_nextPacket);
        Q_ASSERT_X(!_nextPacket,
                   "SendQueue::sendNextPacket()", "Overriden packet in sent list");
    }
    
    bool hasResend = false;
    SequenceNumber seqNum;
    {
        // Check nak list for packet to resend
        QWriteLocker locker(&_naksLock);
        if (!_naks.empty()) {
            hasResend = true;
            seqNum = _naks.front();
            _naks.pop_front();
        }
    }
    
    // Find packet in sent list using SequenceNumber
    if (hasResend) {
        QWriteLocker locker(&_sentLock);
        auto it = _sentPackets.find(seqNum);
        Q_ASSERT_X(it != _sentPackets.end(),
                   "SendQueue::sendNextPacket()", "Couldn't find NAKed packet to resend");
        
        if (it != _sentPackets.end()) {
            it->second.swap(_nextPacket);
            _sentPackets.erase(it);
        }
    }
    
    // If there is no packet to resend, grab the next one in the list
    if (!_nextPacket) {
        QWriteLocker locker(&_packetsLock);
        _nextPacket.swap(_packets.front());
        _packets.pop_front();
    }
    
    // check if we need to fire off a packet pair - we do this
    
    // How long before next packet send
    auto timeToSleep = (sendTime + _packetSendPeriod) - msecTimestampNow(); // msec
    _sendTimer->start(std::max((quint64)0, timeToSleep));
}
