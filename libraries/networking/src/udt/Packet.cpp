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

const qint64 Packet::PACKET_WRITE_ERROR = -1;

qint64 Packet::localHeaderSize(PacketType::Value type) {
    qint64 size = numBytesForArithmeticCodedPacketType(type) + sizeof(PacketVersion) +
                        ((SEQUENCE_NUMBERED_PACKETS.contains(type)) ? sizeof(SequenceNumber) : 0);
    return size;
}

qint64 Packet::maxPayloadSize(PacketType::Value type) {
    return MAX_PACKET_SIZE - localHeaderSize(type);
}

std::unique_ptr<Packet> Packet::create(PacketType::Value type, qint64 size) {
    auto packet = std::unique_ptr<Packet>(new Packet(type, size));
    
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

qint64 Packet::totalHeadersSize() const {
    return localHeaderSize();
}

qint64 Packet::localHeaderSize() const {
    return localHeaderSize(_type);
}

Packet::Packet(PacketType::Value type, qint64 size) :
    _type(type),
    _version(0)
{
    auto maxPayload = maxPayloadSize(type);
    if (size == -1) {
        // default size of -1, means biggest packet possible
        size = maxPayload;
    }

    _packetSize = localHeaderSize(type) + size;
    _packet.reset(new char[_packetSize]);
    _payloadCapacity = size;
    _payloadStart = _packet.get() + (_packetSize - _payloadCapacity);
    
    // Sanity check
    Q_ASSERT(size >= 0 || size < maxPayload);
    
    // copy packet type and version in header
    writePacketTypeAndVersion(type);
    
    // Set control bit and sequence number to 0 if necessary
    if (SEQUENCE_NUMBERED_PACKETS.contains(type)) {
        writeSequenceNumber(0);
    }
}

Packet::Packet(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    _packetSize(size),
    _packet(std::move(data)),
    _senderSockAddr(senderSockAddr)
{
    _type = readType();
    _version = readVersion();
    _payloadCapacity = _packetSize - localHeaderSize(_type);
    _payloadSize = _payloadCapacity;
    _payloadStart = _packet.get() + (_packetSize - _payloadCapacity);
}

Packet::Packet(const Packet& other) :
    QIODevice()
{
    *this = other;
    
    if (other.isOpen()) {
        this->open(other.openMode());
    }
    
    this->seek(other.pos());
}

Packet& Packet::operator=(const Packet& other) {
    _type = other._type;
    
    _packetSize = other._packetSize;
    _packet = std::unique_ptr<char>(new char[_packetSize]);
    memcpy(_packet.get(), other._packet.get(), _packetSize);

    _payloadStart = _packet.get() + (other._payloadStart - other._packet.get());
    _payloadCapacity = other._payloadCapacity;

    _payloadSize = other._payloadSize;

    return *this;
}

Packet::Packet(Packet&& other) {
    *this = std::move(other);
}

Packet& Packet::operator=(Packet&& other) {
    _type = other._type;
    
    _packetSize = other._packetSize;
    _packet = std::move(other._packet);

    _payloadStart = other._payloadStart;
    _payloadCapacity = other._payloadCapacity;

    _payloadSize = other._payloadSize;

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

void Packet::setType(PacketType::Value type) {
    auto currentHeaderSize = totalHeadersSize();
    _type = type;
    writePacketTypeAndVersion(_type);

    // Setting new packet type with a different header size not currently supported
    Q_ASSERT(currentHeaderSize == totalHeadersSize());
    Q_UNUSED(currentHeaderSize);
}

PacketType::Value Packet::readType() const {
    return (PacketType::Value)arithmeticCodingValueFromBuffer(_packet.get());
}

PacketVersion Packet::readVersion() const {
    return *reinterpret_cast<PacketVersion*>(_packet.get() + numBytesForArithmeticCodedPacketType(_type));
}

Packet::SequenceNumber Packet::readSequenceNumber() const {
    if (SEQUENCE_NUMBERED_PACKETS.contains(_type)) {
        SequenceNumber seqNum = *reinterpret_cast<SequenceNumber*>(_packet.get() +
                                                                   numBytesForArithmeticCodedPacketType(_type) +
                                                                   sizeof(PacketVersion));
        return seqNum & ~(1 << 15); // remove control bit
    }
    return -1;
}

bool Packet::readIsControlPacket() const {
    if (SEQUENCE_NUMBERED_PACKETS.contains(_type)) {
        SequenceNumber seqNum = *reinterpret_cast<SequenceNumber*>(_packet.get() +
                                                                   numBytesForArithmeticCodedPacketType(_type) +
                                                                   sizeof(PacketVersion));
        return seqNum & (1 << 15); // Only keep control bit
    }
    return false;
}

void Packet::writePacketTypeAndVersion(PacketType::Value type) {
    // Pack the packet type
    auto offset = packArithmeticallyCodedValue(type, _packet.get());

    // Pack the packet version
    auto version = versionForPacketType(type);
    memcpy(_packet.get() + offset, &version, sizeof(version));
}

void Packet::writeSequenceNumber(SequenceNumber seqNum) {
    // Here we are overriding the control bit to 0.
    // But that is not an issue since we should only ever set the seqNum
    // for data packets going out
    memcpy(_packet.get() + numBytesForArithmeticCodedPacketType(_type) + sizeof(PacketVersion),
           &seqNum, sizeof(seqNum));
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
