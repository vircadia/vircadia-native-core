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
#include <QtCore/QTimer>

#include <SharedUtil.h>

#include "Packet.h"
#include "Socket.h"

namespace udt {

std::unique_ptr<SendQueue> SendQueue::create(Socket* socket, HifiSockAddr dest) {
    auto queue = std::unique_ptr<SendQueue>(new SendQueue(socket, dest));
    
    // Setup queue private thread
    QThread* thread = new QThread(queue.get());
    thread->setObjectName("Networking: SendQueue"); // Name thread for easier debug
    thread->connect(queue.get(), &QObject::destroyed, thread, &QThread::quit); // Thread auto cleanup
    thread->connect(thread, &QThread::finished, thread, &QThread::deleteLater); // Thread auto cleanup
    
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
    
void SendQueue::sendPacket(const Packet& packet) {
    _socket->writeDatagram(packet.getData(), packet.getDataSize(), _destination);
}

void SendQueue::sendNextPacket() {
    if (!_running) {
        return;
    }
    
    // Record timing
    auto sendTime = msecTimestampNow(); // msec
    _lastSendTimestamp = sendTime;
    
    if (_nextPacket) {
        _nextPacket->writeSequenceNumber(++_currentSeqNum);
        sendPacket(*_nextPacket);
        
        // Insert the packet we have just sent in the sent list
        _sentPackets[_nextPacket->getSequenceNumber()].swap(_nextPacket);
        Q_ASSERT(!_nextPacket); // There should be no packet where we inserted
    }
    
    {   // Grab next packet to be sent
        QWriteLocker locker(&_packetsLock);
        _nextPacket.swap(_packets.front());
        _packets.pop_front();
    }
    
    // How long before next packet send
    auto timeToSleep = (sendTime + _packetSendPeriod) - msecTimestampNow(); // msec
    _sendTimer->start(std::max((quint64)0, timeToSleep));
}

}
