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

int Packet::localHeaderSize(bool isPartOfMessage) {
    return sizeof(Packet::SequenceNumberAndBitField) +
            (isPartOfMessage ? sizeof(Packet::MessageNumberAndBitField) : 0);
}

int Packet::totalHeaderSize(bool isPartOfMessage) {
    return BasePacket::totalHeaderSize() + Packet::localHeaderSize(isPartOfMessage);
}

int Packet::maxPayloadSize(bool isPartOfMessage) {
    return BasePacket::maxPayloadSize() - Packet::localHeaderSize(isPartOfMessage);
}


std::unique_ptr<Packet> Packet::create(qint64 size, bool isReliable, bool isPartOfMessage) {
    auto packet = std::unique_ptr<Packet>(new Packet(size, isReliable, isPartOfMessage));
    
    packet->open(QIODevice::ReadWrite);
   
    return packet;
}

std::unique_ptr<Packet> Packet::fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr) {
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

Packet::Packet(qint64 size, bool isReliable, bool isPartOfMessage) :
    BasePacket((size == -1) ? -1 : (Packet::localHeaderSize() + size)),
    _isReliable(isReliable),
    _isPartOfMessage(isPartOfMessage)
{
    adjustPayloadStartAndCapacity(Packet::localHeaderSize(_isPartOfMessage));
    // set the UDT header to default values
    writeHeader();
}

Packet::Packet(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    BasePacket(std::move(data), size, senderSockAddr)
{
    readHeader();

    adjustPayloadStartAndCapacity(Packet::localHeaderSize(_isPartOfMessage), _payloadSize > 0);
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
    _packetPosition = other._packetPosition;
    _messageNumber = other._messageNumber;
}

Packet& Packet::operator=(Packet&& other) {
    BasePacket::operator=(std::move(other));
    
    _isReliable = other._isReliable;
    _isPartOfMessage = other._isPartOfMessage;
    _sequenceNumber = other._sequenceNumber;
    _packetPosition = other._packetPosition;
    _messageNumber = other._messageNumber;

    return *this;
}

void Packet::writeMessageNumber(MessageNumber messageNumber) {
    _isPartOfMessage = true;
    _messageNumber = messageNumber;
    writeHeader();
}

void Packet::writeSequenceNumber(SequenceNumber sequenceNumber) const {
    _sequenceNumber = sequenceNumber;
    writeHeader();
}

static const uint32_t RELIABILITY_BIT_MASK = uint32_t(1) << (SEQUENCE_NUMBER_BITS - 2);
static const uint32_t MESSAGE_BIT_MASK = uint32_t(1) << (SEQUENCE_NUMBER_BITS - 3);
static const uint32_t BIT_FIELD_MASK = CONTROL_BIT_MASK | RELIABILITY_BIT_MASK | MESSAGE_BIT_MASK;

static const uint32_t PACKET_POSITION_MASK = uint32_t(0x03) << 30;
static const uint32_t MESSAGE_NUMBER_MASK = ~PACKET_POSITION_MASK;

void Packet::readHeader() const {
    SequenceNumberAndBitField* seqNumBitField = reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    
    Q_ASSERT_X(!(*seqNumBitField & CONTROL_BIT_MASK), "Packet::readHeader()", "This should be a data packet");
    
    _isReliable = (bool) (*seqNumBitField & RELIABILITY_BIT_MASK); // Only keep reliability bit
    _isPartOfMessage = (bool) (*seqNumBitField & MESSAGE_BIT_MASK); // Only keep message bit
    _sequenceNumber = SequenceNumber{ *seqNumBitField & ~BIT_FIELD_MASK }; // Remove the bit field

    if (_isPartOfMessage) {
        MessageNumberAndBitField* messageNumberAndBitField = seqNumBitField + 1;
        _messageNumber = *messageNumberAndBitField & MESSAGE_NUMBER_MASK;
        _packetPosition = static_cast<PacketPosition>(*messageNumberAndBitField >> 30);
    }
}

void Packet::writeHeader() const {
    // grab pointer to current SequenceNumberAndBitField
    SequenceNumberAndBitField* seqNumBitField = reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    
    // Write sequence number and reset bit field
    Q_ASSERT_X(!((SequenceNumber::Type)_sequenceNumber & BIT_FIELD_MASK),
               "Packet::writeHeader()", "Sequence number is overflowing into bit field");
    *seqNumBitField = ((SequenceNumber::Type)_sequenceNumber);
    
    if (_isReliable) {
        *seqNumBitField |= RELIABILITY_BIT_MASK;
    }
    
    if (_isPartOfMessage) {
        *seqNumBitField |= MESSAGE_BIT_MASK;

        Q_ASSERT_X(!(_messageNumber & PACKET_POSITION_MASK),
                   "Packet::writeHeader()", "Message number is overflowing into bit field");

        MessageNumberAndBitField* messageNumberAndBitField = seqNumBitField + 1;
        *messageNumberAndBitField = _messageNumber;
        *messageNumberAndBitField |= _packetPosition << 30;
    }
}
