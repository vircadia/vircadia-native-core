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
#include <random>
#include <thread>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QJsonObject>
#include <QtCore/QThread>

#include <LogHandler.h>
#include <NumericalConstants.h>
#include <SharedUtil.h>

#include "../NetworkLogging.h"
#include "ControlPacket.h"
#include "Packet.h"
#include "PacketList.h"
#include "../UserActivityLogger.h"
#include "Socket.h"

using namespace udt;
using namespace std::chrono;

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

    // setup psuedo-random number generation for all instances of SendQueue
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<> distribution(0, SequenceNumber::MAX);

    // randomize the intial sequence number
    _initialSequenceNumber = SequenceNumber(distribution(generator));

    // set our member variables from randomized initial number
    _currentSequenceNumber = _initialSequenceNumber - 1;
    _atomicCurrentSequenceNumber = uint32_t(_currentSequenceNumber);
    _lastACKSequenceNumber = uint32_t(_currentSequenceNumber) - 1;

    // default the last receiver response to the current time
    _lastReceiverResponse = QDateTime::currentMSecsSinceEpoch();
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
    
int SendQueue::sendPacket(const Packet& packet) {
    return _socket->writeDatagram(packet.getData(), packet.getDataSize(), _destination);
}
    
void SendQueue::ack(SequenceNumber ack) {
    // this is a response from the client, re-set our timeout expiry and our last response time
    _lastReceiverResponse = QDateTime::currentMSecsSinceEpoch();
    
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

    // call notify_one on the condition_variable_any in case the send thread is sleeping with a full congestion window
    _emptyCondition.notify_one();
}

void SendQueue::nak(SequenceNumber start, SequenceNumber end) {
    // this is a response from the client, re-set our timeout expiry
    _lastReceiverResponse = QDateTime::currentMSecsSinceEpoch(); 
    
    {
        std::lock_guard<std::mutex> nakLocker(_naksLock);
        _naks.insert(start, end);
    }
    
    // call notify_one on the condition_variable_any in case the send thread is sleeping waiting for losses to re-send
    _emptyCondition.notify_one();
}

void SendQueue::fastRetransmit(udt::SequenceNumber ack) {
    {
        std::lock_guard<std::mutex> nakLocker(_naksLock);
        _naks.insert(ack, ack);
    }

    qDebug() << "Added" << (uint32_t) ack << "for fast retransmit";

    // call notify_one on the condition_variable_any in case the send thread is sleeping waiting for losses to re-send
    _emptyCondition.notify_one();
}

void SendQueue::overrideNAKListFromPacket(ControlPacket& packet) {
    // this is a response from the client, re-set our timeout expiry
    _lastReceiverResponse = QDateTime::currentMSecsSinceEpoch();

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
        auto handshakePacket = ControlPacket::create(ControlPacket::Handshake, sizeof(SequenceNumber));

        handshakePacket->writePrimitive(_initialSequenceNumber);
        _socket->writeBasePacket(*handshakePacket, _destination);
        
        // we wait for the ACK or the re-send interval to expire
        static const auto HANDSHAKE_RESEND_INTERVAL = std::chrono::milliseconds(100);
        _handshakeACKCondition.wait_for(handshakeLock, HANDSHAKE_RESEND_INTERVAL);
    }
}

void SendQueue::handshakeACK(SequenceNumber initialSequenceNumber) {
    if (initialSequenceNumber == _initialSequenceNumber) {
        {
            std::lock_guard<std::mutex> locker { _handshakeMutex };
            _hasReceivedHandshakeACK = true;
        }

        // Notify on the handshake ACK condition
        _handshakeACKCondition.notify_one();
    }
}

SequenceNumber SendQueue::getNextSequenceNumber() {
    _atomicCurrentSequenceNumber = (SequenceNumber::Type)++_currentSequenceNumber;
    return _currentSequenceNumber;
}

bool SendQueue::sendNewPacketAndAddToSentList(std::unique_ptr<Packet> newPacket, SequenceNumber sequenceNumber) {
    // write the sequence number and send the packet
    newPacket->writeSequenceNumber(sequenceNumber);

    // Save packet/payload size before we move it
    auto packetSize = newPacket->getWireSize();
    auto payloadSize = newPacket->getPayloadSize();
    
    auto bytesWritten = sendPacket(*newPacket);

    {
        // Insert the packet we have just sent in the sent list
        QWriteLocker locker(&_sentLock);
        auto& entry = _sentPackets[newPacket->getSequenceNumber()];
        entry.first = 0; // No resend
        entry.second.swap(newPacket);
    }
    Q_ASSERT_X(!newPacket, "SendQueue::sendNewPacketAndAddToSentList()", "Overriden packet in sent list");
    
    emit packetSent(packetSize, payloadSize, sequenceNumber, p_high_resolution_clock::now());

    if (bytesWritten < 0) {
        // this is a short-circuit loss - we failed to put this packet on the wire
        // so immediately add it to the loss list

        {
            std::lock_guard<std::mutex> nakLocker(_naksLock);
            _naks.append(sequenceNumber);
        }

        emit shortCircuitLoss(quint32(sequenceNumber));

        return false;
    } else {
        return true;
    }
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

    // Keep an HRC to know when the next packet should have been
    auto nextPacketTimestamp = p_high_resolution_clock::now();

    while (_state == State::Running) {
        bool attemptedToSendPacket = maybeResendPacket();
        
        // if we didn't find a packet to re-send AND we think we can fit a new packet on the wire
        // (this is according to the current flow window size) then we send out a new packet
        auto newPacketCount = 0;
        if (!attemptedToSendPacket) {
            newPacketCount = maybeSendNewPacket();
            attemptedToSendPacket = (newPacketCount > 0);
        }
        
        // since we're a while loop, give the thread a chance to process events
        QCoreApplication::sendPostedEvents(this);
        
        // we just processed events so check now if we were just told to stop
        // If the send queue has been innactive, skip the sleep for
        // Either _isRunning will have been set to false and we'll break
        // Or something happened and we'll keep going
        if (_state != State::Running || isInactive(attemptedToSendPacket)) {
            return;
        }

        // push the next packet timestamp forwards by the current packet send period
        auto nextPacketDelta = (newPacketCount == 2 ? 2 : 1) * _packetSendPeriod;
        nextPacketTimestamp += std::chrono::microseconds(nextPacketDelta);

        // sleep as long as we need for next packet send, if we can
        auto now = p_high_resolution_clock::now();

        auto timeToSleep = duration_cast<microseconds>(nextPacketTimestamp - now);

        // we use nextPacketTimestamp so that we don't fall behind, not to force long sleeps
        // we'll never allow nextPacketTimestamp to force us to sleep for more than nextPacketDelta
        // so cap it to that value
        if (timeToSleep > std::chrono::microseconds(nextPacketDelta)) {
            // reset the nextPacketTimestamp so that it is correct next time we come around
            nextPacketTimestamp = now + std::chrono::microseconds(nextPacketDelta);

            timeToSleep = std::chrono::microseconds(nextPacketDelta);
        }

        // we're seeing SendQueues sleep for a long period of time here,
        // which can lock the NodeList if it's attempting to clear connections
        // for now we guard this by capping the time this thread and sleep for

        const microseconds MAX_SEND_QUEUE_SLEEP_USECS { 2000000 };
        if (timeToSleep > MAX_SEND_QUEUE_SLEEP_USECS) {
            qWarning() << "udt::SendQueue wanted to sleep for" << timeToSleep.count() << "microseconds";
            qWarning() << "Capping sleep to" << MAX_SEND_QUEUE_SLEEP_USECS.count();
            qWarning() << "PSP:" << _packetSendPeriod << "NPD:" << nextPacketDelta
                << "NPT:" << nextPacketTimestamp.time_since_epoch().count()
                << "NOW:" << now.time_since_epoch().count();

            // alright, we're in a weird state
            // we want to know why this is happening so we can implement a better fix than this guard
            // send some details up to the API (if the user allows us) that indicate how we could such a large timeToSleep
            static const QString SEND_QUEUE_LONG_SLEEP_ACTION = "sendqueue-sleep";

            // setup a json object with the details we want
            QJsonObject longSleepObject;
            longSleepObject["timeToSleep"] = qint64(timeToSleep.count());
            longSleepObject["packetSendPeriod"] = _packetSendPeriod.load();
            longSleepObject["nextPacketDelta"] = nextPacketDelta;
            longSleepObject["nextPacketTimestamp"] = qint64(nextPacketTimestamp.time_since_epoch().count());
            longSleepObject["then"] = qint64(now.time_since_epoch().count());

            // hopefully send this event using the user activity logger
            UserActivityLogger::getInstance().logAction(SEND_QUEUE_LONG_SLEEP_ACTION, longSleepObject);

            timeToSleep = MAX_SEND_QUEUE_SLEEP_USECS;
        }

        std::this_thread::sleep_for(timeToSleep);
    }
}

int SendQueue::maybeSendNewPacket() {
    if (!isFlowWindowFull()) {
        // we didn't re-send a packet, so time to send a new one
        
        if (!_packets.isEmpty()) {
            SequenceNumber nextNumber = getNextSequenceNumber();
            
            // grab the first packet we will send
            std::unique_ptr<Packet> firstPacket = _packets.takePacket();
            Q_ASSERT(firstPacket);


            // attempt to send the first packet
            if (sendNewPacketAndAddToSentList(move(firstPacket), nextNumber)) {
                std::unique_ptr<Packet> secondPacket;
                bool shouldSendPairTail = false;

                if (((uint32_t) nextNumber & 0xF) == 0) {
                    // the first packet is the first in a probe pair - every 16 (rightmost 16 bits = 0) packets
                    // pull off a second packet if we can before we unlock
                    shouldSendPairTail = true;

                    secondPacket = _packets.takePacket();
                }

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

                // return the number of attempted packet sends
                return shouldSendPairTail ? 2 : 1;
            } else {
                // we attempted to send a single packet, return 1
                return 1;
            }
        }
    }
    
    // No packets were sent
    return 0;
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

                auto& entry = it->second;
                // we found the packet - grab it
                auto& resendPacket = *(entry.second);
                ++entry.first; // Add 1 resend

                Packet::ObfuscationLevel level = (Packet::ObfuscationLevel)(entry.first < 2 ? 0 : (entry.first - 2) % 4);

                if (level != Packet::NoObfuscation) {
#ifdef UDT_CONNECTION_DEBUG
                    QString debugString = "Obfuscating packet %1 with level %2";
                    debugString = debugString.arg(QString::number((uint32_t)resendPacket.getSequenceNumber()),
                                                  QString::number(level));
                    if (resendPacket.isPartOfMessage()) {
                        debugString += "\n";
                        debugString += "    Message Number: %1, Part Number: %2.";
                        debugString = debugString.arg(QString::number(resendPacket.getMessageNumber()),
                                                      QString::number(resendPacket.getMessagePartNumber()));
                    }
                    static QString repeatedMessage = LogHandler::getInstance().addRepeatedMessageRegex("^Obfuscating packet .*");
                    qCritical() << qPrintable(debugString);
#endif

                    // Create copy of the packet
                    auto packet = Packet::createCopy(resendPacket);

                    // unlock the sent packets
                    sentLocker.unlock();

                    // Obfuscate packet
                    packet->obfuscate(level);

                    // send it off
                    sendPacket(*packet);
                } else {
                    // send it off
                    sendPacket(resendPacket);

                    // unlock the sent packets
                    sentLocker.unlock();
                }
                
                emit packetRetransmitted(resendPacket.getWireSize(), it->first, p_high_resolution_clock::now());
                
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

bool SendQueue::isInactive(bool attemptedToSendPacket) {
    // check for connection timeout first

    // that will be the case if we have had 16 timeouts since hearing back from the client, and it has been
    // at least 5 seconds
    static const int NUM_TIMEOUTS_BEFORE_INACTIVE = 16;
    static const int MIN_MS_BEFORE_INACTIVE = 5 * 1000;

    auto sinceLastResponse = (QDateTime::currentMSecsSinceEpoch() - _lastReceiverResponse);

    if (sinceLastResponse > 0 &&
        sinceLastResponse >= int64_t(NUM_TIMEOUTS_BEFORE_INACTIVE * (_estimatedTimeout / USECS_PER_MSEC)) &&
        sinceLastResponse > MIN_MS_BEFORE_INACTIVE) {
        // If the flow window has been full for over CONSIDER_INACTIVE_AFTER,
        // then signal the queue is inactive and return so it can be cleaned up

#ifdef UDT_CONNECTION_DEBUG
        qCDebug(networking) << "SendQueue to" << _destination << "reached" << NUM_TIMEOUTS_BEFORE_INACTIVE << "timeouts"
            << "and" << MIN_MS_BEFORE_INACTIVE << "milliseconds before receiving any ACK/NAK and is now inactive. Stopping.";
#endif

        deactivate();
        return true;
    }

    if (!attemptedToSendPacket) {
        // During our processing above we didn't send any packets
        
        // If that is still the case we should use a condition_variable_any to sleep until we have data to handle.
        // To confirm that the queue of packets and the NAKs list are still both empty we'll need to use the DoubleLock
        using DoubleLock = DoubleLock<std::recursive_mutex, std::mutex>;
        DoubleLock doubleLock(_packets.getLock(), _naksLock);
        DoubleLock::Lock locker(doubleLock, std::try_to_lock);
        
        if (locker.owns_lock() && (_packets.isEmpty() || isFlowWindowFull()) && _naks.isEmpty()) {
            // The packets queue and loss list mutexes are now both locked and they're both empty
            
            if (uint32_t(_lastACKSequenceNumber) == uint32_t(_currentSequenceNumber)) {
                // we've sent the client as much data as we have (and they've ACKed it)
                // either wait for new data to send or 5 seconds before cleaning up the queue
                static const auto EMPTY_QUEUES_INACTIVE_TIMEOUT = std::chrono::seconds(5);
                
                // use our condition_variable_any to wait
                auto cvStatus = _emptyCondition.wait_for(locker, EMPTY_QUEUES_INACTIVE_TIMEOUT);
                
                if (cvStatus == std::cv_status::timeout && (_packets.isEmpty() || isFlowWindowFull()) && _naks.isEmpty()) {
#ifdef UDT_CONNECTION_DEBUG
                    qCDebug(networking) << "SendQueue to" << _destination << "has been empty for"
                        << EMPTY_QUEUES_INACTIVE_TIMEOUT.count()
                        << "seconds and receiver has ACKed all packets."
                        << "The queue is now inactive and will be stopped.";
#endif

                    // we have the lock again - Make sure to unlock it
                    locker.unlock();
                    
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
                
                if (cvStatus == std::cv_status::timeout && (_packets.isEmpty() || isFlowWindowFull()) && _naks.isEmpty()
                    && SequenceNumber(_lastACKSequenceNumber) < _currentSequenceNumber) {
                    // after a timeout if we still have sent packets that the client hasn't ACKed we
                    // add them to the loss list
                    
                    // Note that thanks to the DoubleLock we have the _naksLock right now
                    _naks.append(SequenceNumber(_lastACKSequenceNumber) + 1, _currentSequenceNumber);

                    // we have the lock again - time to unlock it
                    locker.unlock();
                    
                    emit timeout();
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

bool SendQueue::isFlowWindowFull() const {
    return seqlen(SequenceNumber { (uint32_t) _lastACKSequenceNumber }, _currentSequenceNumber)  > _flowWindowSize;
}
