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

const qint64 Packet::PACKET_WRITE_ERROR = -1;

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
    return localHeaderSize();
}

qint64 Packet::localHeaderSize() const {
    return localHeaderSize(_isPartOfMessage);
}

Packet::Packet(qint64 size, bool isReliable, bool isPartOfMessage) :
    _isReliable(isReliable),
    _isPartOfMessage(isPartOfMessage)
{
    auto maxPayload = maxPayloadSize();
    
    if (size == -1) {
        // default size of -1, means biggest packet possible
        size = maxPayload;
    }

    _packetSize = localHeaderSize() + size;
    _packet.reset(new char[_packetSize]);
    _payloadCapacity = size;
    _payloadStart = _packet.get() + (_packetSize - _payloadCapacity);
    
    // Sanity check
    Q_ASSERT(size >= 0 || size < maxPayload);
    
    // set the UDT header to default values
    writeSequenceNumber(0);
}

Packet::Packet(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    _packetSize(size),
    _packet(std::move(data)),
    _senderSockAddr(senderSockAddr)
{
    readIsReliable();
    readIsPartOfMessage();
    readSequenceNumber();
    
    _payloadCapacity = _packetSize - localHeaderSize();
    _payloadSize = _payloadCapacity;
    _payloadStart = _packet.get() + (_packetSize - _payloadCapacity);
}

Packet::Packet(const Packet& other) :
    QIODevice()
{
    *this = other;
}

Packet& Packet::operator=(const Packet& other) {
    _packetSize = other._packetSize;
    _packet = std::unique_ptr<char>(new char[_packetSize]);
    memcpy(_packet.get(), other._packet.get(), _packetSize);

    _payloadStart = _packet.get() + (other._payloadStart - other._packet.get());
    _payloadCapacity = other._payloadCapacity;

    _payloadSize = other._payloadSize;
    
    if (other.isOpen() && !isOpen()) {
        open(other.openMode());
    }
    
    seek(other.pos());

    return *this;
}

Packet::Packet(Packet&& other) {
    *this = std::move(other);
}

Packet& Packet::operator=(Packet&& other) {
    _packetSize = other._packetSize;
    _packet = std::move(other._packet);

    _payloadStart = other._payloadStart;
    _payloadCapacity = other._payloadCapacity;

    _payloadSize = other._payloadSize;
    
    _senderSockAddr = std::move(other._senderSockAddr);
    
    if (other.isOpen() && !isOpen()) {
        open(other.openMode());
    }
    
    seek(other.pos());

    return *this;
}

void Packet::setPayloadSize(qint64 payloadSize) {
    if (isWritable()) {
        Q_ASSERT(payloadSize <= _payloadCapacity);
        _payloadSize = payloadSize;
    } else {
        qDebug() << "You can not call setPayloadSize for a non-writeable Packet.";
        Q_ASSERT(false);
    }
}

bool Packet::reset() {
    if (isWritable()) {
        _payloadSize = 0;
    }
    
    return QIODevice::reset();
}

static const uint32_t CONTROL_BIT_MASK = 1 << (sizeof(Packet::SequenceNumberAndBitField) - 1);
static const uint32_t RELIABILITY_BIT_MASK = 1 << (sizeof(Packet::SequenceNumberAndBitField) - 2);
static const uint32_t MESSAGE_BIT_MASK = 1 << (sizeof(Packet::SequenceNumberAndBitField) - 3);
static const int BIT_FIELD_LENGTH = 3;
static const uint32_t BIT_FIELD_MASK = CONTROL_BIT_MASK | RELIABILITY_BIT_MASK | MESSAGE_BIT_MASK;

void Packet::readIsReliable() {
    SequenceNumberAndBitField seqNumBitField = *reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    _isReliable = (bool) (seqNumBitField & RELIABILITY_BIT_MASK); // Only keep reliability bit
}

void Packet::readIsPartOfMessage() {
    SequenceNumberAndBitField seqNumBitField = *reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    _isReliable = (bool) (seqNumBitField & MESSAGE_BIT_MASK); // Only keep message bit
}

void Packet::readSequenceNumber() {
    SequenceNumberAndBitField seqNumBitField = *reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    _sequenceNumber = (seqNumBitField & ~BIT_FIELD_MASK); // Remove the bit field
}

static const uint32_t MAX_SEQUENCE_NUMBER = UINT32_MAX >> BIT_FIELD_LENGTH;

void Packet::writeSequenceNumber(SequenceNumber sequenceNumber) {
    // make sure this is a sequence number <= 29 bit unsigned max (536,870,911)
    Q_ASSERT(sequenceNumber <= MAX_SEQUENCE_NUMBER);
    
    // grab pointer to current SequenceNumberAndBitField
    SequenceNumberAndBitField* seqNumBitField = reinterpret_cast<SequenceNumberAndBitField*>(_packet.get());
    
    // write new value by ORing (old value & BIT_FIELD_MASK) with new seqNum
    *seqNumBitField = (*seqNumBitField & BIT_FIELD_MASK) | sequenceNumber;
}

QByteArray Packet::read(qint64 maxSize) {
    qint64 sizeToRead = std::min(size() - pos(), maxSize);
    QByteArray data { QByteArray::fromRawData(getPayload() + pos(), sizeToRead) };
    seek(pos() + sizeToRead);
    return data;
}

qint64 Packet::writeData(const char* data, qint64 maxSize) {
    
    // make sure we have the space required to write this block
    if (maxSize <= bytesAvailableForWrite()) {
        qint64 currentPos = pos();
        
        Q_ASSERT(currentPos < _payloadCapacity);

        // good to go - write the data
        memcpy(_payloadStart + currentPos, data, maxSize);

        // keep track of _payloadSize so we can just write the actual data when packet is about to be sent
        _payloadSize = std::max(currentPos + maxSize, _payloadSize);

        // return the number of bytes written
        return maxSize;
    } else {
        // not enough space left for this write - return an error
        return PACKET_WRITE_ERROR;
    }
}

qint64 Packet::readData(char* dest, qint64 maxSize) {
    // we're either reading what is left from the current position or what was asked to be read
    qint64 numBytesToRead = std::min(bytesLeftToRead(), maxSize);

    if (numBytesToRead > 0) {
        int currentPosition = pos();

        // read out the data
        memcpy(dest, _payloadStart + currentPosition, numBytesToRead);
    }

    return numBytesToRead;
}
