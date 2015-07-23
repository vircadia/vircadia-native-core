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

#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <SharedUtil.h>

namespace udt {

std::unique_ptr<SendQueue> SendQueue::create() {
    return std::unique_ptr<SendQueue>(new SendQueue());
}
    
SendQueue::SendQueue() {
    _sendTimer.reset(new QTimer(this));
    _sendTimer->setSingleShot(true);
    QObject::connect(_sendTimer.get(), &QTimer::timeout, this, &SendQueue::sendNextPacket);
    
    _packetSendPeriod = DEFAULT_SEND_PERIOD;
    _lastSendTimestamp = 0;
}

void SendQueue::queuePacket(std::unique_ptr<Packet> packet) {
    QWriteLocker locker(&_packetsLock);
    _packets.push_back(std::move(packet));
}

void SendQueue::start() {
    // We need to make sure this is called on the right thread
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "start", Qt::QueuedConnection);
    }
    // This will send a packet and fire the send timer
    sendNextPacket();
}

void SendQueue::stop() {
    // We need to make sure this is called on the right thread
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "stop", Qt::QueuedConnection);
    }
    // Stopping the timer will stop the sending of packets
    _sendTimer->stop();
}

void SendQueue::sendNextPacket() {
    // Record timing
    auto sendTime = msecTimestampNow(); // msec
    _lastSendTimestamp = sendTime;
    // TODO send packet
    
    // Insert the packet we have just sent in the sent list
    _sentPackets[_nextPacket->readSequenceNumber()].swap(_nextPacket);
    Q_ASSERT(!_nextPacket); // There should be no packet where we inserted
    
    {   // Grab next packet to be sent
        QWriteLocker locker(&_packetsLock);
        _nextPacket.swap(_packets.front());
        _packets.pop_front();
    }
    
    // How long before next packet send
    auto timeToSleep = (sendTime + _packetSendPeriod) - msecTimestampNow(); // msec
    if (timeToSleep > 0) {
        _sendTimer->start(timeToSleep);
    } else {
        _sendTimer->start(0);
    }
}

}
