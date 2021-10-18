//
//  Connection.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 7/27/15.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Connection.h"

#include <random>

#include <QtCore/QThread>

#include <NumericalConstants.h>

#include "../SockAddr.h"
#include "../NetworkLogging.h"

#include "CongestionControl.h"
#include "ControlPacket.h"
#include "Packet.h"
#include "PacketList.h"
#include "Socket.h"
#include <Trace.h>

using namespace udt;
using namespace std::chrono;

Connection::Connection(Socket* parentSocket, SockAddr destination, std::unique_ptr<CongestionControl> congestionControl) :
    _parentSocket(parentSocket),
    _destination(destination),
    _congestionControl(move(congestionControl))
{
    Q_ASSERT_X(parentSocket, "Connection::Connection", "Must be called with a valid Socket*");
    
    Q_ASSERT_X(_congestionControl, "Connection::Connection", "Must be called with a valid CongestionControl object");
    _congestionControl->init();

    // Setup packets
    static const int ACK_PACKET_PAYLOAD_BYTES = sizeof(SequenceNumber);
    static const int HANDSHAKE_ACK_PAYLOAD_BYTES = sizeof(SequenceNumber);

    _ackPacket = ControlPacket::create(ControlPacket::ACK, ACK_PACKET_PAYLOAD_BYTES);
    _handshakeACK = ControlPacket::create(ControlPacket::HandshakeACK, HANDSHAKE_ACK_PAYLOAD_BYTES);


    // setup psuedo-random number generation shared by all connections
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<> distribution(0, SequenceNumber::MAX);

    // randomize the initial sequence number
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
        QObject::connect(this, &Connection::destinationAddressChange, _sendQueue.get(), &SendQueue::updateDestinationAddress);

        
        // set defaults on the send queue from our congestion control object and estimatedTimeout()
        _sendQueue->setPacketSendPeriod(_congestionControl->_packetSendPeriod);
        _sendQueue->setEstimatedTimeout(_congestionControl->estimatedTimeout());
        _sendQueue->setFlowWindowSize(_congestionControl->_congestionWindowSize);

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
}

void Connection::recordSentPackets(int wireSize, int payloadSize,
                                   SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) {
    _stats.recordSentPackets(payloadSize, wireSize);

    _congestionControl->onPacketSent(wireSize, seqNum, timePoint);
}

void Connection::recordRetransmission(int wireSize, int payloadSize,
                                      SequenceNumber seqNum, p_high_resolution_clock::time_point timePoint) {
    _stats.recordRetransmittedPackets(payloadSize, wireSize);

    _congestionControl->onPacketReSent(wireSize, seqNum, timePoint);
}

void Connection::recordSentUnreliablePackets(int wireSize, int payloadSize) {
    _stats.recordUnreliableSentPackets(payloadSize, wireSize);
}

void Connection::recordReceivedUnreliablePackets(int wireSize, int payloadSize) {
    _stats.recordUnreliableReceivedPackets(payloadSize, wireSize);
}

void Connection::sendACK() {
    SequenceNumber nextACKNumber = nextACK();

    // we have received new packets since the last sent ACK
    // or our congestion control dictates that we always send ACKs

    _ackPacket->reset(); // We need to reset it every time.

    // pack in the ACK number
    _ackPacket->writePrimitive(nextACKNumber);

    // have the socket send off our packet
    _parentSocket->writeBasePacket(*_ackPacket, _destination);
    
    _stats.recordSentACK(_ackPacket->getWireSize());
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
        // Update largest received sequence number
        _lastReceivedSequenceNumber = sequenceNumber;
    } else {
        // Otherwise, it could be a resend, try and remove it from the loss list
        wasDuplicate = !_lossList.remove(sequenceNumber);
    }

    // using a congestion control that ACKs every packet (like TCP Vegas)
    sendACK();
    
    if (wasDuplicate) {
        _stats.recordDuplicatePackets(payloadSize, packetSize);
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

                qCDebug(networking) << "Got HandshakeRequest from" << _destination << "while active, stopping SendQueue";
                _hasReceivedHandshakeACK = false;
                stopSendQueue();
            }
            break;
    }
}

void Connection::processACK(ControlPacketPointer controlPacket) {
    // read the ACKed sequence number
    SequenceNumber ack;
    controlPacket->readPrimitive(&ack);
    
    // update the total count of received ACKs
    _stats.recordReceivedACK(controlPacket->getWireSize());
    
    // validate that this isn't a BS ACK
    if (ack > getSendQueue().getCurrentSequenceNumber()) {
        // in UDT they specifically break the connection here - do we want to do anything?
        Q_ASSERT_X(false, "Connection::processACK", "ACK received higher than largest sent sequence number");
        return;
    }
    
    if (ack < _lastReceivedACK) {
        // this is an out of order ACK, bail
        return;
    }

    if (ack > _lastReceivedACK) {
        // this is not a repeated ACK, so update our member and tell the send queue
        _lastReceivedACK = ack;

        // ACK the send queue so it knows what was received
        getSendQueue().ack(ack);
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
    
    // clear the loss list
    _lossList.clear();
    
    // clear any pending received messages
    for (auto& pendingMessage : _pendingReceivedMessages) {
        _parentSocket->messageFailed(this, pendingMessage.first);
    }
    _pendingReceivedMessages.clear();
}

void Connection::updateCongestionControlAndSendQueue(std::function<void ()> congestionCallback) {
    // update the last sent sequence number in congestion control
    _congestionControl->setSendCurrentSequenceNumber(getSendQueue().getCurrentSequenceNumber());
    
    // fire congestion control callback
    congestionCallback();
    
    auto& sendQueue = getSendQueue();
    
    // now that we've updated the congestion control, update the packet send period and flow window size
    sendQueue.setPacketSendPeriod(_congestionControl->_packetSendPeriod);
    sendQueue.setEstimatedTimeout(_congestionControl->estimatedTimeout());
    sendQueue.setFlowWindowSize(_congestionControl->_congestionWindowSize);
    
    // record connection stats
    _stats.recordPacketSendPeriod(_congestionControl->_packetSendPeriod);
    _stats.recordCongestionWindowSize(_congestionControl->_congestionWindowSize);
}

void PendingReceivedMessage::enqueuePacket(std::unique_ptr<Packet> packet) {
    Q_ASSERT_X(packet->isPartOfMessage(),
               "PendingReceivedMessage::enqueuePacket",
               "called with a packet that is not part of a message");
    
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

void Connection::setDestinationAddress(const SockAddr& destination) {
    if (_destination != destination) {
        _destination = destination;
        emit destinationAddressChange(destination);
    }
}
