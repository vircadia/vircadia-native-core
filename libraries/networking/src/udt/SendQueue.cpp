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
#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QThread>

#include <SharedUtil.h>

#include "../NetworkLogging.h"
#include "ControlPacket.h"
#include "Packet.h"
#include "PacketList.h"
#include "Socket.h"

using namespace udt;

template <typename Mutex1, typename Mutex2>
class DoubleLock {
public:
    using Lock = std::unique_lock<DoubleLock<Mutex1, Mutex2>>;
    
    DoubleLock(Mutex1& mutex1, Mutex2& mutex2) : _mutex1(mutex1), _mutex2(mutex2) { }
    
    DoubleLock(const DoubleLock&) = delete;
    DoubleLock& operator=(const DoubleLock&) = delete;
    
    // Either locks all the mutexes or none of them
    bool try_lock() { return (std::try_lock(_mutex1, _mutex2) == -1); }
    
    // Locks all the mutexes
    void lock() { std::lock(_mutex1, _mutex2); }
    
     // Undefined behavior if not locked
    void unlock() { _mutex1.unlock(); _mutex2.unlock(); }
    
private:
    Mutex1& _mutex1;
    Mutex2& _mutex2;
};

std::unique_ptr<SendQueue> SendQueue::create(Socket* socket, HifiSockAddr destination) {
    Q_ASSERT_X(socket, "SendQueue::create", "Must be called with a valid Socket*");
    
    auto queue = std::unique_ptr<SendQueue>(new SendQueue(socket, destination));
    
    // Setup queue private thread
    QThread* thread = new QThread;
    thread->setObjectName("Networking: SendQueue " + destination.objectName()); // Name thread for easier debug
    
    connect(thread, &QThread::started, queue.get(), &SendQueue::run);
    
    connect(queue.get(), &QObject::destroyed, thread, &QThread::quit); // Thread auto cleanup
    connect(thread, &QThread::finished, thread, &QThread::deleteLater); // Thread auto cleanup
    
    // Move queue to private thread and start it
    queue->moveToThread(thread);
    
    thread->start();
    
    return queue;
}
    
SendQueue::SendQueue(Socket* socket, HifiSockAddr dest) :
    _socket(socket),
    _destination(dest)
{
}

void SendQueue::queuePacket(std::unique_ptr<Packet> packet) {
    _packets.queuePacket(std::move(packet));
    
    // call notify_one on the condition_variable_any in case the send thread is sleeping waiting for packets
    _emptyCondition.notify_one();
    
    if (!this->thread()->isRunning() && _state == State::NotStarted) {
        this->thread()->start();
    }
}

void SendQueue::queuePacketList(std::unique_ptr<PacketList> packetList) {
    _packets.queuePacketList(std::move(packetList));
    
    // call notify_one on the condition_variable_any in case the send thread is sleeping waiting for packets
    _emptyCondition.notify_one();
    
    if (!this->thread()->isRunning() && _state == State::NotStarted) {
        this->thread()->start();
    }
}

void SendQueue::stop() {
    
    _state = State::Stopped;
    
    // Notify all conditions in case we're waiting somewhere
    _handshakeACKCondition.notify_one();
    _emptyCondition.notify_one();
}
    
void SendQueue::sendPacket(const Packet& packet) {
    _socket->writeDatagram(packet.getData(), packet.getDataSize(), _destination);
}
    
void SendQueue::ack(SequenceNumber ack) {
    // this is a response from the client, re-set our timeout expiry and our last response time
    _timeoutExpiryCount = 0;
    _lastReceiverResponse = uint64_t(QDateTime::currentMSecsSinceEpoch());
    
    if (_lastACKSequenceNumber == (uint32_t) ack) {
        return;
    }
    
    {
        // remove any ACKed packets from the map of sent packets
        QWriteLocker locker(&_sentLock);
        for (auto seq = SequenceNumber { (uint32_t) _lastACKSequenceNumber }; seq <= ack; ++seq) {
            _sentPackets.erase(seq);
        }
    }
    
    {   // remove any sequence numbers equal to or lower than this ACK in the loss list
        std::lock_guard<std::mutex> nakLocker(_naksLock);
        
        if (!_naks.isEmpty() && _naks.getFirstSequenceNumber() <= ack) {
            _naks.remove(_naks.getFirstSequenceNumber(), ack);
        }
    }
    
    _lastACKSequenceNumber = (uint32_t) ack;
}

void SendQueue::nak(SequenceNumber start, SequenceNumber end) {
    // this is a response from the client, re-set our timeout expiry
    _timeoutExpiryCount = 0;
    _lastReceiverResponse = uint64_t(QDateTime::currentMSecsSinceEpoch());
    
    {
        std::lock_guard<std::mutex> nakLocker(_naksLock);
        _naks.insert(start, end);
    }
    
    // call notify_one on the condition_variable_any in case the send thread is sleeping waiting for losses to re-send
    _emptyCondition.notify_one();
}

void SendQueue::overrideNAKListFromPacket(ControlPacket& packet) {
    // this is a response from the client, re-set our timeout expiry
    _timeoutExpiryCount = 0;
    _lastReceiverResponse = uint64_t(QDateTime::currentMSecsSinceEpoch());
    
    {
        std::lock_guard<std::mutex> nakLocker(_naksLock);
        _naks.clear();
        
        SequenceNumber first, second;
        while (packet.bytesLeftToRead() >= (qint64)(2 * sizeof(SequenceNumber))) {
            packet.readPrimitive(&first);
            packet.readPrimitive(&second);
            
            if (first == second) {
                _naks.append(first);
            } else {
                _naks.append(first, second);
            }
        }
    }
    
    // call notify_one on the condition_variable_any in case the send thread is sleeping waiting for losses to re-send
    _emptyCondition.notify_one();
}

void SendQueue::sendHandshake() {
    std::unique_lock<std::mutex> handshakeLock { _handshakeMutex };
    if (!_hasReceivedHandshakeACK) {
        // we haven't received a handshake ACK from the client, send another now
        static const auto handshakePacket = ControlPacket::create(ControlPacket::Handshake, 0);
        _socket->writeBasePacket(*handshakePacket, _destination);
        
        // we wait for the ACK or the re-send interval to expire
        static const auto HANDSHAKE_RESEND_INTERVAL = std::chrono::milliseconds(100);
        _handshakeACKCondition.wait_for(handshakeLock, HANDSHAKE_RESEND_INTERVAL);
    }
}

void SendQueue::handshakeACK() {
    {
        std::lock_guard<std::mutex> locker { _handshakeMutex };
        _hasReceivedHandshakeACK = true;
    }
    
    // Notify on the handshake ACK condition
    _handshakeACKCondition.notify_one();
}

SequenceNumber SendQueue::getNextSequenceNumber() {
    _atomicCurrentSequenceNumber = (SequenceNumber::Type)++_currentSequenceNumber;
    return _currentSequenceNumber;
}

void SendQueue::sendNewPacketAndAddToSentList(std::unique_ptr<Packet> newPacket, SequenceNumber sequenceNumber) {
    // write the sequence number and send the packet
    newPacket->writeSequenceNumber(sequenceNumber);
    sendPacket(*newPacket);
    
    // Save packet/payload size before we move it
    auto packetSize = newPacket->getDataSize();
    auto payloadSize = newPacket->getPayloadSize();
    
    {
        // Insert the packet we have just sent in the sent list
        QWriteLocker locker(&_sentLock);
        _sentPackets[newPacket->getSequenceNumber()].swap(newPacket);
    }
    Q_ASSERT_X(!newPacket, "SendQueue::sendNewPacketAndAddToSentList()", "Overriden packet in sent list");
    
    emit packetSent(packetSize, payloadSize);
}

void SendQueue::run() {
    if (_state == State::Stopped) {
        // we've already been asked to stop before we even got a chance to start
        // don't start now
#ifdef UDT_CONNECTION_DEBUG
        qDebug() << "SendQueue asked to run after being told to stop. Will not run.";
#endif
        return;
    } else if (_state == State::Running) {
#ifdef UDT_CONNECTION_DEBUG
        qDebug() << "SendQueue asked to run but is already running (according to state). Will not re-run.";
#endif
        return;
    }
    
    _state = State::Running;
    
    // Wait for handshake to be complete
    while (_state == State::Running && !_hasReceivedHandshakeACK) {
        sendHandshake();

        // Keep processing events
        QCoreApplication::sendPostedEvents(this);
        
        // Once we're here we've either received the handshake ACK or it's going to be time to re-send a handshake.
        // Either way let's continue processing - no packets will be sent if no handshake ACK has been received.
    }
        
    while (_state == State::Running) {
        // Record how long the loop takes to execute
        const auto loopStartTimestamp = p_high_resolution_clock::now();
        
        bool sentAPacket = maybeResendPacket();
        
        // if we didn't find a packet to re-send AND we think we can fit a new packet on the wire
        // (this is according to the current flow window size) then we send out a new packet
        if (!sentAPacket) {
            sentAPacket = maybeSendNewPacket();
        }
        
        // since we're a while loop, give the thread a chance to process events
        QCoreApplication::sendPostedEvents(this);
        
        // we just processed events so check now if we were just told to stop
        // If the send queue has been innactive, skip the sleep for
        // Either _isRunning will have been set to false and we'll break
        // Or something happened and we'll keep going
        if (_state != State::Running || isInactive(sentAPacket)) {
            return;
        }
        
        // sleep as long as we need until next packet send, if we can
        const auto loopEndTimestamp = p_high_resolution_clock::now();
        const auto timeToSleep = (loopStartTimestamp + std::chrono::microseconds(_packetSendPeriod)) - loopEndTimestamp;
        std::this_thread::sleep_for(timeToSleep);
    }
}

bool SendQueue::maybeSendNewPacket() {
    if (seqlen(SequenceNumber { (uint32_t) _lastACKSequenceNumber }, _currentSequenceNumber) <= _flowWindowSize) {
        // we didn't re-send a packet, so time to send a new one
        
        
        if (!_packets.isEmpty()) {
            SequenceNumber nextNumber = getNextSequenceNumber();
            
            // grab the first packet we will send
            std::unique_ptr<Packet> firstPacket = _packets.takePacket();
            Q_ASSERT(firstPacket);
            
            std::unique_ptr<Packet> secondPacket;
            bool shouldSendPairTail = false;
            
            if (((uint32_t) nextNumber & 0xF) == 0) {
                // the first packet is the first in a probe pair - every 16 (rightmost 16 bits = 0) packets
                // pull off a second packet if we can before we unlock
                shouldSendPairTail = true;
                
                secondPacket = _packets.takePacket();
            }
            
            // definitely send the first packet
            sendNewPacketAndAddToSentList(move(firstPacket), nextNumber);
            
            // do we have a second in a pair to send as well?
            if (secondPacket) {
                sendNewPacketAndAddToSentList(move(secondPacket), getNextSequenceNumber());
            } else if (shouldSendPairTail) {
                // we didn't get a second packet to send in the probe pair
                // send a control packet of type ProbePairTail so the receiver can still do
                // proper bandwidth estimation
                static auto pairTailPacket = ControlPacket::create(ControlPacket::ProbeTail);
                _socket->writeBasePacket(*pairTailPacket, _destination);
            }
            
            // We sent our packet(s), return here
            return true;
        }
    }
    // No packets were sent
    return false;
}

bool SendQueue::maybeResendPacket() {
    
    // the following while makes sure that we find a packet to re-send, if there is one
    while (true) {
        
        std::unique_lock<std::mutex> naksLocker(_naksLock);
        
        if (!_naks.isEmpty()) {
            // pull the sequence number we need to re-send
            SequenceNumber resendNumber = _naks.popFirstSequenceNumber();
            naksLocker.unlock();
            
            // pull the packet to re-send from the sent packets list
            QReadLocker sentLocker(&_sentLock);
            
            // see if we can find the packet to re-send
            auto it = _sentPackets.find(resendNumber);
            
            if (it != _sentPackets.end()) {
                // we found the packet - grab it
                auto& resendPacket = *(it->second);
                
                // send it off
                sendPacket(resendPacket);
                
                // unlock the sent packets
                sentLocker.unlock();
                
                emit packetRetransmitted();
                
                // Signal that we did resend a packet
                return true;
            } else {
                // we didn't find this packet in the sentPackets queue - assume this means it was ACKed
                // we'll fire the loop again to see if there is another to re-send
                continue;
            }
        }
        
        // break from the while, we didn't resend a packet
        break;
    }
    
    // No packet was resent
    return false;
}

bool SendQueue::isInactive(bool sentAPacket) {
    if (!sentAPacket) {
        // check if it is time to break this connection
        
        // that will be the case if we have had 16 timeouts since hearing back from the client, and it has been
        // at least 5 seconds
        static const int NUM_TIMEOUTS_BEFORE_INACTIVE = 16;
        static const int MIN_SECONDS_BEFORE_INACTIVE_MS = 5 * 1000;
        if (_timeoutExpiryCount >= NUM_TIMEOUTS_BEFORE_INACTIVE &&
            (QDateTime::currentMSecsSinceEpoch() - _lastReceiverResponse) > MIN_SECONDS_BEFORE_INACTIVE_MS) {
            // If the flow window has been full for over CONSIDER_INACTIVE_AFTER,
            // then signal the queue is inactive and return so it can be cleaned up
            
#ifdef UDT_CONNECTION_DEBUG
            qCDebug(networking) << "SendQueue to" << _destination << "reached" << NUM_TIMEOUTS_BEFORE_INACTIVE << "timeouts"
            << "and 5s before receiving any ACK/NAK and is now inactive. Stopping.";
#endif
            
            deactivate();
            return true;
        }
        
        // During our processing above we didn't send any packets
        
        // If that is still the case we should use a condition_variable_any to sleep until we have data to handle.
        // To confirm that the queue of packets and the NAKs list are still both empty we'll need to use the DoubleLock
        using DoubleLock = DoubleLock<std::recursive_mutex, std::mutex>;
        DoubleLock doubleLock(_packets.getLock(), _naksLock);
        DoubleLock::Lock locker(doubleLock, std::try_to_lock);
        
        if (locker.owns_lock() && _packets.isEmpty() && _naks.isEmpty()) {
            // The packets queue and loss list mutexes are now both locked and they're both empty
            
            if (uint32_t(_lastACKSequenceNumber) == uint32_t(_currentSequenceNumber)) {
                // we've sent the client as much data as we have (and they've ACKed it)
                // either wait for new data to send or 5 seconds before cleaning up the queue
                static const auto EMPTY_QUEUES_INACTIVE_TIMEOUT = std::chrono::seconds(5);
                
                // use our condition_variable_any to wait
                auto cvStatus = _emptyCondition.wait_for(locker, EMPTY_QUEUES_INACTIVE_TIMEOUT);
                
                // we have the lock again - Make sure to unlock it
                locker.unlock();
                
                if (cvStatus == std::cv_status::timeout) {
#ifdef UDT_CONNECTION_DEBUG
                    qCDebug(networking) << "SendQueue to" << _destination << "has been empty for"
                    << EMPTY_QUEUES_INACTIVE_TIMEOUT.count()
                    << "seconds and receiver has ACKed all packets."
                    << "The queue is now inactive and will be stopped.";
#endif
                    
                    // Deactivate queue
                    deactivate();
                    return true;
                }
            } else {
                // We think the client is still waiting for data (based on the sequence number gap)
                // Let's wait either for a response from the client or until the estimated timeout
                // (plus the sync interval to allow the client to respond) has elapsed
                auto waitDuration = std::chrono::microseconds(_estimatedTimeout + _syncInterval);
                
                // use our condition_variable_any to wait
                auto cvStatus = _emptyCondition.wait_for(locker, waitDuration);
                
                if (cvStatus == std::cv_status::timeout) {
                    // increase the number of timeouts
                    ++_timeoutExpiryCount;
                    
                    if (SequenceNumber(_lastACKSequenceNumber) < _currentSequenceNumber) {
                        // after a timeout if we still have sent packets that the client hasn't ACKed we
                        // add them to the loss list
                        
                        // Note that thanks to the DoubleLock we have the _naksLock right now
                        _naks.append(SequenceNumber(_lastACKSequenceNumber) + 1, _currentSequenceNumber);
                    }
                }
            }
        }
    }
    
    return false;
}

void SendQueue::deactivate() {
    // this queue is inactive - emit that signal and stop the while
    emit queueInactive();
    
    _state = State::Stopped;
}
