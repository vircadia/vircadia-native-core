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

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

#include <SharedUtil.h>

#include "ControlPacket.h"
#include "Packet.h"
#include "Socket.h"

using namespace udt;
using namespace std::chrono;

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
    _packetSendPeriod = DEFAULT_SEND_PERIOD;
}

void SendQueue::queuePacket(std::unique_ptr<Packet> packet) {
    {
        QWriteLocker locker(&_packetsLock);
        _packets.push_back(std::move(packet));
    }
    if (!_isRunning) {
        run();
    }
}

void SendQueue::run() {
    // We need to make sure this is called on the right thread
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "run", Qt::QueuedConnection);
    }
    _isRunning = true;
    
    // This will loop and sleep to send packets
    loop();
}

void SendQueue::stop() {
    _isRunning = false;
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
    
    {   // remove any ACKed packets from the map of sent packets
        QWriteLocker locker(&_sentLock);
        for (auto seq = _lastAck; seq <= ack; ++seq) {
            _sentPackets.erase(seq);
        }
    }
    
    {   // remove any sequence numbers equal to or lower than this ACK in the loss list
        QWriteLocker nakLocker(&_naksLock);
        _naks.remove(_naks.getFirstSequenceNumber(), ack);
    }
    
    _lastAck = ack;
}

void SendQueue::nak(SequenceNumber start, SequenceNumber end) {
    QWriteLocker locker(&_naksLock);
    _naks.insert(start, end);
}

void SendQueue::overrideNAKListFromPacket(ControlPacket& packet) {
    QWriteLocker locker(&_naksLock);
    _naks.clear();
    
    SequenceNumber first, second;
    while (packet.bytesLeftToRead() > (qint64)(2 * sizeof(SequenceNumber))) {
        packet.readPrimitive(&first);
        packet.readPrimitive(&second);
        
        if (first == second) {
            _naks.append(first);
        } else {
            _naks.append(first, second);
        }
    }
}

SequenceNumber SendQueue::getNextSequenceNumber() {
    _atomicCurrentSequenceNumber = (SequenceNumber::Type)++_currentSequenceNumber;
    return _currentSequenceNumber;
}

void SendQueue::loop() {
    while (_isRunning) {
        // Record timing
        _lastSendTimestamp = high_resolution_clock::now();
        
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
            if (_naks.getLength() > 0) {
                hasResend = true;
                seqNum = _naks.popFirstSequenceNumber();
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
        
        // since we're a while loop, give the thread a chance to process events
        QCoreApplication::processEvents();
        
        // we just processed events so check now if we were just told to stop
        if (!_isRunning) {
            break;
        }
        
        // sleep as long as we need until next packet send, if we can
        auto now = high_resolution_clock::now();
        auto microsecondDuration = (_lastSendTimestamp + microseconds(_packetSendPeriod)) - now;
        
        if (microsecondDuration.count() > 0) {
            usleep(microsecondDuration.count());
        }
    }
}
