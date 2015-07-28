//
//  Packet.cpp
//  libraries/networking/src
//
//  Created by Clement on 7/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Packet.h"

using namespace udt;

std::unique_ptr<Packet> Packet::create(qint64 size, bool isReliable, bool isPartOfMessage) {
    auto packet = std::unique_ptr<Packet>(new Packet(size, isReliable, isPartOfMessage));
    
    packet->open(QIODevice::ReadWrite);
   
    return packet;
}

std::unique_ptr<Packet> Packet::fromReceivedPacket(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr) {
    // Fail with invalid size
    Q_ASSERT(size >= 0);

    // allocate memory
    auto packet = std::unique_ptr<Packet>(new Packet(std::move(data), size, senderSockAddr));

    packet->open(QIODevice::ReadOnly);

    return packet;
}

std::unique_ptr<Packet> Packet::createCopy(const Packet& other) {
    return std::unique_ptr<Packet>(new Packet(other));
}

qint64 Packet::maxPayloadSize(bool isPartOfMessage) {
    return MAX_PACKET_SIZE - localHeaderSize(isPartOfMessage);
}

qint64 Packet::localHeaderSize(bool isPartOfMessage) {
    return sizeof(SequenceNumberAndBitField) + (isPartOfMessage ? sizeof(MessageNumberAndBitField) : 0);
}

qint64 Packet::maxPayloadSize() const {
    return MAX_PACKET_SIZE - localHeaderSize();
}

qint64 Packet::totalHeadersSize() const {
    return BasePacket::localHeaderSize() + localHeaderSize();
}

qint64 Packet::localHeaderSize() const {
    return localHeaderSize(_isPartOfMessage);
}

Packet::Packet(qint64 size, bool isReliable, bool isPartOfMessage) :
    BasePacket(size),
    _isReliable(isReliable),
    _isPartOfMessage(isPartOfMessage)
{
    adjustPayloadStartAndCapacity();
    
    // set the UDT header to default values
    writeHeader();
}

Packet::Packet(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    BasePacket(std::move(data), size, senderSockAddr)
{
    readHeader();

    adjustPayloadStartAndCapacity(_payloadSize > 0);
}

Packet::Packet(const Packet& other) :
    BasePacket(other)
{
    _isReliable = other._isReliable;
    _isPartOfMessage = other._isPartOfMessage;
    _sequenceNumber = other._sequenceNumber;
}

Packet& Packet::operator=(const Packet& other) {
    BasePacket::operator=(other);
    
    _isReliable = other._isReliable;
    _isPartOfMessage = other._isPartOfMessage;
    _sequenceNumber = other._sequenceNumber;

    return *this;
}

Packet::Packet(Packet&& other) :
    BasePacket(std::move(other))
{
    _isReliable = other._isReliable;
    _isPartOfMessage = other._isPartOfMessage;
    _sequenceNumber = other._sequenceNumber;
}

Packet& Packet::operator=(Packet&& other) {
    BasePacket::operator=(std::move(other));
    
    _isReliable = other._isReliable;
    _isPartOfMessage = other._isPartOfMessage;
    _sequenceNumber = other._sequenceNumber;

    return *this;
}

void Packet::setSequenceNumber(SequenceNumber sequenceNumber) {
    _sequenceNumber = sequenceNumber;
    writeHeader();
}

static const uint32_t CONTROL_BIT_MASK = 1 << (sizeof(Packet::SequenceNumberAndBitField) - 1);
static const uint32_t RELIABILITY_BIT_MASK = 1 << (sizeof(Packet::SequenceNumberAndBitField) - 2);
static const uint32_t MESSAGE_BIT_MASK = 1 << (sizeof(Packet::SequenceNumberAndBitField) - 3);
static const uint32_t BIT_FIELD_MASK = CONTROL_BIT_MASK | RELIABILITY_BIT_MASK | MESSAGE_BIT_MASK;

void Packet::readHeader() {
    SequenceNumberAndBitField seqNumBitField = *reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    Q_ASSERT_X((bool) (seqNumBitField & CONTROL_BIT_MASK),
               "Packet::readHeader()", "This should be a data packet");
    _isReliable = (bool) (seqNumBitField & RELIABILITY_BIT_MASK); // Only keep reliability bit
    _isPartOfMessage = (bool) (seqNumBitField & MESSAGE_BIT_MASK); // Only keep message bit
    _sequenceNumber = SequenceNumber{ seqNumBitField & ~BIT_FIELD_MASK }; // Remove the bit field
}

void Packet::writeHeader() {
    // grab pointer to current SequenceNumberAndBitField
    SequenceNumberAndBitField* seqNumBitField = reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    
    // 0 for data packets
    *seqNumBitField &= ~CONTROL_BIT_MASK;
    
    if (_isPartOfMessage) {
        *seqNumBitField |= MESSAGE_BIT_MASK;
    } else {
        *seqNumBitField &= ~MESSAGE_BIT_MASK;
    }
    
    if (_isReliable) {
        *seqNumBitField |= RELIABILITY_BIT_MASK;
    } else {
        *seqNumBitField &= ~RELIABILITY_BIT_MASK;
    }
    
    // write new value by ORing (old value & BIT_FIELD_MASK) with new seqNum
    *seqNumBitField = (*seqNumBitField & BIT_FIELD_MASK) | (SequenceNumber::Type)_sequenceNumber;
}
