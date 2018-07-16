//
//  Connection.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 7/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Connection.h"

#include <random>

#include <QtCore/QThread>

#include <NumericalConstants.h>

#include "../HifiSockAddr.h"
#include "../NetworkLogging.h"

#include "CongestionControl.h"
#include "ControlPacket.h"
#include "Packet.h"
#include "PacketList.h"
#include "Socket.h"
#include <Trace.h>

using namespace udt;
using namespace std::chrono;

Connection::Connection(Socket* parentSocket, HifiSockAddr destination, std::unique_ptr<CongestionControl> congestionControl) :
    _parentSocket(parentSocket),
    _destination(destination),
    _congestionControl(move(congestionControl))
{
    Q_ASSERT_X(parentSocket, "Connection::Connection", "Must be called with a valid Socket*");
    
    Q_ASSERT_X(_congestionControl, "Connection::Connection", "Must be called with a valid CongestionControl object");
    _congestionControl->init();
    
    // setup default SYN, RTT and RTT Variance based on the SYN interval in CongestionControl object
    _synInterval = _congestionControl->synInterval();
    
    resetRTT();
    
    // set the initial RTT and flow window size on congestion control object
    _congestionControl->setRTT(_rtt);
    _congestionControl->setMaxCongestionWindowSize(_flowWindowSize);

    // Setup packets
    static const int ACK_PACKET_PAYLOAD_BYTES = sizeof(_lastSentACK)
                            + sizeof(_rtt) + sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t);
    static const int HANDSHAKE_ACK_PAYLOAD_BYTES = sizeof(SequenceNumber);

    _ackPacket = ControlPacket::create(ControlPacket::ACK, ACK_PACKET_PAYLOAD_BYTES);
    _handshakeACK = ControlPacket::create(ControlPacket::HandshakeACK, HANDSHAKE_ACK_PAYLOAD_BYTES);


    // setup psuedo-random number generation shared by all connections
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<> distribution(0, SequenceNumber::MAX);

    // randomize the intial sequence number
    _initialSequenceNumber = SequenceNumber(distribution(generator));
}

Connection::~Connection() {
    stopSendQueue();

    // Fail any pending received messages
    for (auto& pendingMessage : _pendingReceivedMessages) {
        _parentSocket->messageFailed(this, pendingMessage.first);
    }
}

void Connection::stopSendQueue() {
    if (auto sendQueue = _sendQueue.release()) {
        // grab the send queue thread so we can wait on it
        QThread* sendQueueThread = sendQueue->thread();
        
        // tell the send queue to stop and be deleted
        
        sendQueue->stop();

        _lastMessageNumber = sendQueue->getCurrentMessageNumber();

        sendQueue->deleteLater();
        
        // wait on the send queue thread so we know the send queue is gone
        sendQueueThread->quit();
        sendQueueThread->wait();
    }
}

void Connection::resetRTT() {
    _rtt = _synInterval * 10;
    _rttVariance = _rtt / 2;
}

void Connection::setMaxBandwidth(int maxBandwidth) {
    _congestionControl->setMaxBandwidth(maxBandwidth);
}

SendQueue& Connection::getSendQueue() {
    if (!_sendQueue) {
        // we may have a sequence number from the previous inactive queue - re-use that so that the
        // receiver is getting the sequence numbers it expects (given that the connection must still be active)

        // Lasily create send queue

        if (!_hasReceivedHandshakeACK) {
            // First time creating a send queue for this connection
            _sendQueue = SendQueue::create(_parentSocket, _destination, _initialSequenceNumber - 1, _lastMessageNumber, _hasReceivedHandshakeACK);
            _lastReceivedACK = _sendQueue->getCurrentSequenceNumber();
        } else {
            // Connection already has a handshake from a previous send queue
            _sendQueue = SendQueue::create(_parentSocket, _destination, _lastReceivedACK, _lastMessageNumber, _hasReceivedHandshakeACK);
        }

#ifdef UDT_CONNECTION_DEBUG
        qCDebug(networking) << "Created SendQueue for connection to" << _destination;
#endif
        
        QObject::connect(_sendQueue.get(), &SendQueue::packetSent, this, &Connection::packetSent);
        QObject::connect(_sendQueue.get(), &SendQueue::packetSent, this, &Connection::recordSentPackets);
        QObject::connect(_sendQueue.get(), &SendQueue::packetRetransmitted, this, &Connection::recordRetransmission);
        QObject::connect(_sendQueue.get(), &SendQueue::queueInactive, this, &Connection::queueInactive);
        QObject::connect(_sendQueue.get(), &SendQueue::timeout, this, &Connection::queueTimeout);

        
        // set defaults on the send queue from our congestion control object and estimatedTimeout()
        _sendQueue->setPacketSendPeriod(_congestionControl->_packetSendPeriod);
        _sendQueue->setSyncInterval(_synInterval);
        _sendQueue->setEstimatedTimeout(estimatedTimeout());
        _sendQueue->setFlowWindowSize(std::min(_flowWindowSize, (int) _congestionControl->_congestionWindowSize));

        // give the randomized sequence number to the congestion control object
        _congestionControl->setInitialSendSequenceNumber(_sendQueue->getCurrentSequenceNumber());
    }
    
    return *_sendQueue;
}

void Connection::queueInactive() {
    // tell our current send queue to go down and reset our ptr to it to null
    stopSendQueue();
    
#ifdef UDT_CONNECTION_DEBUG
    qCDebug(networking) << "Connection to" << _destination << "has stopped its SendQueue.";
#endif
}

void Connection::queueTimeout() {
    updateCongestionControlAndSendQueue([this] {
        _congestionControl->onTimeout();
    });
}

void Connection::sendReliablePacket(std::unique_ptr<Packet> packet) {
    Q_ASSERT_X(packet->isReliable(), "Connection::send", "Trying to send an unreliable packet reliably.");
    getSendQueue().queuePacket(std::move(packet));
}

void Connection::sendReliablePacketList(std::unique_ptr<PacketList> packetList) {
    Q_ASSERT_X(packetList->isReliable(), "Connection::send", "Trying to send an unreliable packet reliably.");
    getSendQueue().queuePacketList(std::move(packetList));
}

void Connection::queueReceivedMessagePacket(std::unique_ptr<Packet> packet) {
    Q_ASSERT(packet->isPartOfMessage());

    auto messageNumber = packet->getMessageNumber();
    auto& pendingMessage = _pendingReceivedMessages[messageNumber];

    pendingMessage.enqueuePacket(std::move(packet));

    bool processedLastOrOnly = false;

    while (pendingMessage.hasAvailablePackets()) {
        auto packet = pendingMessage.removeNextPacket();

        auto packetPosition = packet->getPacketPosition();

        _parentSocket->messageReceived(std::move(packet));

        // if this was the last or only packet, then we can remove the pending message from our hash
        if (packetPosition == Packet::PacketPosition::LAST ||
            packetPosition == Packet::PacketPosition::ONLY) {
            processedLastOrOnly = true;
        }
    }

    if (processedLastOrOnly) {
        _pendingReceivedMessages.erase(messageNumber);
    }
}

void Connection::sync() {
    if (_isReceivingData) {
        
        // check if we should expire the receive portion of this connection
        // this occurs if it has been 16 timeouts since the last data received and at least 5 seconds
        static const int NUM_TIMEOUTS_BEFORE_EXPIRY = 16;
        static const int MIN_SECONDS_BEFORE_EXPIRY = 5;
        
        auto now = p_high_resolution_clock::now();
        
        auto sincePacketReceive = now - _lastReceiveTime;
        
        if (duration_cast<microseconds>(sincePacketReceive).count() >= NUM_TIMEOUTS_BEFORE_EXPIRY * estimatedTimeout()
            && duration_cast<seconds>(sincePacketReceive).count() >= MIN_SECONDS_BEFORE_EXPIRY ) {
            // the receive side of this connection is expired
            _isReceivingData = false;
        }
    }
}

void Connection::recordSentPackets(int wireSize, int payloadSize,
                                   SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) {
    _stats.recordSentPackets(payloadSize, wireSize);

    _congestionControl->onPacketSent(wireSize, seqNum, timePoint);
}

void Connection::recordRetransmission(int wireSize, SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) {
    _stats.record(ConnectionStats::Stats::Retransmission);

    _congestionControl->onPacketSent(wireSize, seqNum, timePoint);
}

void Connection::sendACK(bool wasCausedBySyncTimeout) {
    static p_high_resolution_clock::time_point lastACKSendTime;
    
    SequenceNumber nextACKNumber = nextACK();
    Q_ASSERT_X(nextACKNumber >= _lastSentACK, "Connection::sendACK", "Sending lower ACK, something is wrong");

    // we have received new packets since the last sent ACK
    // or our congestion control dictates that we always send ACKs
    
    // update the last sent ACK
    _lastSentACK = nextACKNumber;

    _ackPacket->reset(); // We need to reset it every time.

    // pack in the ACK number
    _ackPacket->writePrimitive(nextACKNumber);
    
    // pack in the RTT and variance
    _ackPacket->writePrimitive(_rtt);
    
    // pack the available buffer size, in packets
    // in our implementation we have no hard limit on receive buffer size, send the default value
    _ackPacket->writePrimitive((int32_t) udt::MAX_PACKETS_IN_FLIGHT);

    if (wasCausedBySyncTimeout) {
        // grab the up to date packet receive speed
        int32_t packetReceiveSpeed = _receiveWindow.getPacketReceiveSpeed();

        // update those values in our connection stats
        _stats.recordReceiveRate(packetReceiveSpeed);
        
        // pack in the receive speed
        _ackPacket->writePrimitive(packetReceiveSpeed);
    }
    
    // record this as the last ACK send time
    lastACKSendTime = p_high_resolution_clock::now();
    
    // have the socket send off our packet
    _parentSocket->writeBasePacket(*_ackPacket, _destination);
    
    _stats.record(ConnectionStats::Stats::SentACK);
}

SequenceNumber Connection::nextACK() const {
    if (_lossList.getLength() > 0) {
        return _lossList.getFirstSequenceNumber() - 1;
    } else {
        return _lastReceivedSequenceNumber;
    }
}

void Connection::sendHandshakeRequest() {
    auto handshakeRequestPacket = ControlPacket::create(ControlPacket::HandshakeRequest, 0);
    _parentSocket->writeBasePacket(*handshakeRequestPacket, _destination);

    _didRequestHandshake = true;
}

bool Connection::processReceivedSequenceNumber(SequenceNumber sequenceNumber, int packetSize, int payloadSize) {
    if (!_hasReceivedHandshake) {
        // Refuse to process any packets until we've received the handshake
        // Send handshake request to re-request a handshake

#ifdef UDT_CONNECTION_DEBUG
        qCDebug(networking) << "Received packet before receiving handshake, sending HandshakeRequest";
#endif

        sendHandshakeRequest();

        return false;
    }
    
    _isReceivingData = true;
    
    // mark our last receive time as now (to push the potential expiry farther)
    _lastReceiveTime = p_high_resolution_clock::now();

    _receiveWindow.onPacketArrival();
    
    // If this is not the next sequence number, report loss
    if (sequenceNumber > _lastReceivedSequenceNumber + 1) {
        if (_lastReceivedSequenceNumber + 1 == sequenceNumber - 1) {
            _lossList.append(_lastReceivedSequenceNumber + 1);
        } else {
            _lossList.append(_lastReceivedSequenceNumber + 1, sequenceNumber - 1);
        }
    }
    
    bool wasDuplicate = false;
    
    if (sequenceNumber > _lastReceivedSequenceNumber) {
        // Update largest recieved sequence number
        _lastReceivedSequenceNumber = sequenceNumber;
    } else {
        // Otherwise, it could be a resend, try and remove it from the loss list
        wasDuplicate = !_lossList.remove(sequenceNumber);
    }

    // using a congestion control that ACKs every packet (like TCP Vegas)
    sendACK(true);
    
    if (wasDuplicate) {
        _stats.record(ConnectionStats::Stats::Duplicate);
    } else {
        _stats.recordReceivedPackets(payloadSize, packetSize);
    }

    return !wasDuplicate;
}

void Connection::processControl(ControlPacketPointer controlPacket) {
    
    // Simple dispatch to control packets processing methods based on their type.
    
    // Processing of control packets (other than Handshake / Handshake ACK)
    // is not performed if the handshake has not been completed.
    
    switch (controlPacket->getType()) {
        case ControlPacket::ACK:
            if (_hasReceivedHandshakeACK) {
                processACK(move(controlPacket));
            }
            break;
        case ControlPacket::Handshake:
            processHandshake(move(controlPacket));
            break;
        case ControlPacket::HandshakeACK:
            processHandshakeACK(move(controlPacket));
            break;
        case ControlPacket::HandshakeRequest:
            if (_hasReceivedHandshakeACK) {
                // We're already in a state where we've received a handshake ack, so we are likely in a state
                // where the other end expired our connection. Let's reset.

#ifdef UDT_CONNECTION_DEBUG
                qCDebug(networking) << "Got  HandshakeRequest from" << _destination << ", stopping SendQueue";
#endif
                _hasReceivedHandshakeACK = false;
                stopSendQueue();
            }
            break;
        case ControlPacket::LightACK:
        case ControlPacket::ACK2:
        case ControlPacket::NAK:
        case ControlPacket::TimeoutNAK:
        case ControlPacket::ProbeTail:
            break;
    }
}

void Connection::processACK(ControlPacketPointer controlPacket) {
    // read the ACKed sequence number
    SequenceNumber ack;
    controlPacket->readPrimitive(&ack);
    
    // update the total count of received ACKs
    _stats.record(ConnectionStats::Stats::ReceivedACK);
    
    // validate that this isn't a BS ACK
    if (ack > getSendQueue().getCurrentSequenceNumber()) {
        // in UDT they specifically break the connection here - do we want to do anything?
        Q_ASSERT_X(false, "Connection::processACK", "ACK recieved higher than largest sent sequence number");
        return;
    }
    
    // read the RTT
    int32_t rtt;
    controlPacket->readPrimitive(&rtt);
    
    if (ack < _lastReceivedACK) {
        // this is an out of order ACK, bail
        return;
    }
    
    // this is a valid ACKed sequence number - update the flow window size and the last received ACK
    int32_t packedFlowWindow;
    controlPacket->readPrimitive(&packedFlowWindow);
    
    _flowWindowSize = packedFlowWindow;
    
    if (ack == _lastReceivedACK) {
        // processing an already received ACK, bail
        return;
    }
    
    _lastReceivedACK = ack;
    
    // ACK the send queue so it knows what was received
    getSendQueue().ack(ack);
    
    // update the RTT
    updateRTT(rtt);
    
    // write this RTT to stats
    _stats.recordRTT(rtt);
    
    // set the RTT for congestion control
    _congestionControl->setRTT(_rtt);
    
    if (controlPacket->bytesLeftToRead() > 0) {
        int32_t receiveRate;
        
        Q_ASSERT_X(controlPacket->bytesLeftToRead() == sizeof(receiveRate),
                   "Connection::processACK", "sync interval ACK packet does not contain expected data");
        
        controlPacket->readPrimitive(&receiveRate);
        
        // set the delivery rate for congestion control
        // these are calculated using an EWMA
        static const int EMWA_ALPHA_NUMERATOR = 8;
        
        // record these samples in connection stats
        _stats.recordSendRate(receiveRate);
        
        _deliveryRate = (_deliveryRate * (EMWA_ALPHA_NUMERATOR - 1) + receiveRate) / EMWA_ALPHA_NUMERATOR;
        
        _congestionControl->setReceiveRate(_deliveryRate);
    }
    
    // give this ACK to the congestion control and update the send queue parameters
    updateCongestionControlAndSendQueue([this, ack, &controlPacket] {
        if (_congestionControl->onACK(ack, controlPacket->getReceiveTime())) {
            // the congestion control has told us it needs a fast re-transmit of ack + 1, add that now
            _sendQueue->fastRetransmit(ack + 1);
        }
    });
    
    _stats.record(ConnectionStats::Stats::ProcessedACK);
}

void Connection::processHandshake(ControlPacketPointer controlPacket) {
    SequenceNumber initialSequenceNumber;
    controlPacket->readPrimitive(&initialSequenceNumber);
    
    if (!_hasReceivedHandshake || initialSequenceNumber != _initialReceiveSequenceNumber) {
        // server sent us a handshake - we need to assume this means state should be reset
        // as long as we haven't received a handshake yet or we have and we've received some data

#ifdef UDT_CONNECTION_DEBUG
        if (initialSequenceNumber != _initialReceiveSequenceNumber) {
            qCDebug(networking) << "Resetting receive state, received a new initial sequence number in handshake";
        }
#endif
        resetReceiveState();
        _initialReceiveSequenceNumber = initialSequenceNumber;
        _lastReceivedSequenceNumber = initialSequenceNumber - 1;
        _lastSentACK = initialSequenceNumber - 1;
    }

    _handshakeACK->reset();
    _handshakeACK->writePrimitive(initialSequenceNumber);
    _parentSocket->writeBasePacket(*_handshakeACK, _destination);
    
    // indicate that handshake has been received
    _hasReceivedHandshake = true;

    if (_didRequestHandshake) {
        emit receiverHandshakeRequestComplete(_destination);
        _didRequestHandshake = false;
    }
}

void Connection::processHandshakeACK(ControlPacketPointer controlPacket) {
    // if we've decided to clean up the send queue then this handshake ACK should be ignored, it's useless
    if (_sendQueue) {
        SequenceNumber initialSequenceNumber;
        controlPacket->readPrimitive(&initialSequenceNumber);

        if (initialSequenceNumber == _initialSequenceNumber) {
            // hand off this handshake ACK to the send queue so it knows it can start sending
            getSendQueue().handshakeACK();

            // indicate that handshake ACK was received
            _hasReceivedHandshakeACK = true;
        }
    }
}

void Connection::resetReceiveState() {
    
    // reset all SequenceNumber member variables back to default
    SequenceNumber defaultSequenceNumber;
    
    _lastReceivedSequenceNumber = defaultSequenceNumber;

    _lastSentACK = defaultSequenceNumber;
    
    // clear the loss list
    _lossList.clear();
    
    // clear sync variables
    _isReceivingData = false;
    _connectionStart = p_high_resolution_clock::now();
    
    // reset RTT to initial value
    resetRTT();
    
    // clear the intervals in the receive window
    _receiveWindow.reset();
    
    // clear any pending received messages
    for (auto& pendingMessage : _pendingReceivedMessages) {
        _parentSocket->messageFailed(this, pendingMessage.first);
    }
    _pendingReceivedMessages.clear();
}

void Connection::updateRTT(int rtt) {
    // This updates the RTT using exponential weighted moving average
    // This is the Jacobson's forumla for RTT estimation
    // http://www.mathcs.emory.edu/~cheung/Courses/455/Syllabus/7-transport/Jacobson-88.pdf
    
    // Estimated RTT = (1 - x)(estimatedRTT) + (x)(sampleRTT)
    // (where x = 0.125 via Jacobson)
    
    // Deviation  = (1 - x)(deviation) + x |sampleRTT - estimatedRTT|
    // (where x = 0.25 via Jacobson)
    
    static const int RTT_ESTIMATION_ALPHA_NUMERATOR = 8;
    static const int RTT_ESTIMATION_VARIANCE_ALPHA_NUMERATOR = 4;
   
    _rtt = (_rtt * (RTT_ESTIMATION_ALPHA_NUMERATOR - 1) + rtt) / RTT_ESTIMATION_ALPHA_NUMERATOR;
    
    _rttVariance = (_rttVariance * (RTT_ESTIMATION_VARIANCE_ALPHA_NUMERATOR - 1)
                    + abs(rtt - _rtt)) / RTT_ESTIMATION_VARIANCE_ALPHA_NUMERATOR;
}

int Connection::estimatedTimeout() const {
    return _rtt + _rttVariance * 4;
}

void Connection::updateCongestionControlAndSendQueue(std::function<void ()> congestionCallback) {
    // update the last sent sequence number in congestion control
    _congestionControl->setSendCurrentSequenceNumber(getSendQueue().getCurrentSequenceNumber());
    
    // fire congestion control callback
    congestionCallback();
    
    auto& sendQueue = getSendQueue();
    
    // now that we've updated the congestion control, update the packet send period and flow window size
    sendQueue.setPacketSendPeriod(_congestionControl->_packetSendPeriod);
    sendQueue.setEstimatedTimeout(estimatedTimeout());
    sendQueue.setFlowWindowSize(std::min(_flowWindowSize, (int) _congestionControl->_congestionWindowSize));
    
    // record connection stats
    _stats.recordPacketSendPeriod(_congestionControl->_packetSendPeriod);
    _stats.recordCongestionWindowSize(_congestionControl->_congestionWindowSize);
}

void PendingReceivedMessage::enqueuePacket(std::unique_ptr<Packet> packet) {
    Q_ASSERT_X(packet->isPartOfMessage(),
               "PendingReceivedMessage::enqueuePacket",
               "called with a packet that is not part of a message");
    
    if (packet->getPacketPosition() == Packet::PacketPosition::LAST ||
        packet->getPacketPosition() == Packet::PacketPosition::ONLY) {
        _hasLastPacket = true;
        _numPackets = packet->getMessagePartNumber() + 1;
    }

    // Insert into the packets list in sorted order. Because we generally expect to receive packets in order, begin
    // searching from the end of the list.
    auto messagePartNumber = packet->getMessagePartNumber();
    auto it = std::find_if(_packets.rbegin(), _packets.rend(),
        [&](const std::unique_ptr<Packet>& value) { return messagePartNumber >= value->getMessagePartNumber(); });

    if (it != _packets.rend() && ((*it)->getMessagePartNumber() == messagePartNumber)) {
        qCDebug(networking) << "PendingReceivedMessage::enqueuePacket: This is a duplicate packet";
        return;
    }
    
    _packets.insert(it.base(), std::move(packet));
}

bool PendingReceivedMessage::hasAvailablePackets() const {
    return _packets.size() > 0
        && _nextPartNumber == _packets.front()->getMessagePartNumber();
}

std::unique_ptr<Packet> PendingReceivedMessage::removeNextPacket() {
    if (hasAvailablePackets()) {
        _nextPartNumber++;
        auto p = std::move(_packets.front());
        _packets.pop_front();
        return p;
    }
    return std::unique_ptr<Packet>();
}
