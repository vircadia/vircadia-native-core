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

#include "LimitedNodeList.h"

qint64 Packet::localHeaderSize(PacketType::Value type) {
    qint64 size = numBytesForArithmeticCodedPacketType(type) + sizeof(PacketVersion) +
                        ((SEQUENCE_NUMBERED_PACKETS.contains(type)) ? sizeof(SequenceNumber) : 0);
    return size;
}

qint64 Packet::maxPayloadSize(PacketType::Value type) {
    return MAX_PACKET_SIZE - localHeaderSize(type);
}

std::unique_ptr<Packet> Packet::create(PacketType::Value type, qint64 size) {
    auto maxPayload = maxPayloadSize(type);
    if (size == -1) {
        // default size of -1, means biggest packet possible
        size = maxPayload;
    }
    if (size <= 0 || size > maxPayload) {
        // Invalid size, return null pointer
        return std::unique_ptr<Packet>();
    }
    
    // allocate memory
    return std::unique_ptr<Packet>(new Packet(type, size));
}

qint64 Packet::totalHeadersSize() const {
    return localHeaderSize();
}

qint64 Packet::localHeaderSize() const {
    return localHeaderSize(_type);
}

Packet::Packet(PacketType::Value type, qint64 size) :
    _type(type),
    _packetSize(localHeaderSize(_type) + size),
    _packet(new char(_packetSize)),
    _payloadStart(_packet.get() + localHeaderSize(_type)),
    _capacity(size) {
        // Sanity check
        Q_ASSERT(size <= maxPayloadSize(type));
        
        // copy packet type and version in header
        setPacketTypeAndVersion(type);
        
        // Set control bit and sequence number to 0 if necessary
        if (SEQUENCE_NUMBERED_PACKETS.contains(type)) {
            setSequenceNumber(0);
        }
}

Packet::Packet(const Packet& other) {
    *this = other;
}

Packet& Packet::operator=(const Packet& other) {
    _packetSize = other._packetSize;
    _packet = std::unique_ptr<char>(new char(_packetSize));
    memcpy(_packet.get(), other._packet.get(), _packetSize);
    
    _payloadStart = _packet.get() + (other._payloadStart - other._packet.get());
    _position = other._position;
    _capacity = other._capacity;
    
    _sizeUsed = other._sizeUsed;
    
    return *this;
}

Packet::Packet(Packet&& other) {
    *this = std::move(other);
}

Packet& Packet::operator=(Packet&& other) {
    _packetSize = other._packetSize;
    _packet = std::move(other._packet);
    
    _payloadStart = other._payloadStart;
    _position = other._position;
    _capacity = other._capacity;
    
    _sizeUsed = other._sizeUsed;
    
    return *this;
}

PacketType::Value Packet::getPacketType() const {
    return (PacketType::Value)arithmeticCodingValueFromBuffer(_packet.get());
}

PacketVersion Packet::getPacketTypeVersion() const {
    return *reinterpret_cast<PacketVersion*>(_packet.get() + numBytesForArithmeticCodedPacketType(_type));
}

Packet::SequenceNumber Packet::getSequenceNumber() const {
    if (SEQUENCE_NUMBERED_PACKETS.contains(_type)) {
        SequenceNumber seqNum = *reinterpret_cast<SequenceNumber*>(_packet.get() +
                                                                   numBytesForArithmeticCodedPacketType(_type) +
                                                                   sizeof(PacketVersion));
        return seqNum & ~(1 << 15); // remove control bit
    }
    return -1;
}

bool Packet::isControlPacket() const {
    if (SEQUENCE_NUMBERED_PACKETS.contains(_type)) {
        SequenceNumber seqNum = *reinterpret_cast<SequenceNumber*>(_packet.get() +
                                                                   numBytesForArithmeticCodedPacketType(_type) +
                                                                   sizeof(PacketVersion));
        return seqNum & (1 << 15); // Only keep control bit
    }
    return false;
}

void Packet::setPacketTypeAndVersion(PacketType::Value type) {
    // Pack the packet type
    auto offset = packArithmeticallyCodedValue(type, _packet.get());
    
    // Pack the packet version
    auto version { versionForPacketType(type) };
    memcpy(_packet.get() + offset, &version, sizeof(version));
}

void Packet::setSequenceNumber(SequenceNumber seqNum) {
    // Here we are overriding the control bit to 0.
    // But that is not an issue since we should only ever set the seqNum
    // for data packets going out
    memcpy(_packet.get() + numBytesForArithmeticCodedPacketType(_type) + sizeof(PacketVersion),
           &seqNum, sizeof(seqNum));
}

bool Packet::seek(qint64 pos) {
    bool valid = (pos >= 0) && (pos < size());
    if (valid) {
        _position = pos;
    }
    return valid;
}

static const qint64 PACKET_WRITE_ERROR = -1;
qint64 Packet::writeData(const char* data, qint64 maxSize) {
    // make sure we have the space required to write this block
    if (maxSize <= bytesAvailable()) {
        qint64 currentPos = pos();
        
        // good to go - write the data
        memcpy(_payloadStart + currentPos, data, maxSize);
        
        // seek to the new position based on where our write just finished
        seek(currentPos + maxSize);
        
        // keep track of _sizeUsed so we can just write the actual data when packet is about to be sent
        _sizeUsed = std::max(pos() + 1, _sizeUsed);
        
        // return the number of bytes written
        return maxSize;
    } else {
        // not enough space left for this write - return an error
        return PACKET_WRITE_ERROR;
    }
}

qint64 Packet::readData(char* dest, qint64 maxSize) {
    // we're either reading what is left from the current position or what was asked to be read
    qint64 numBytesToRead = std::min(bytesAvailable(), maxSize);
    
    if (numBytesToRead > 0) {
        int currentPosition = pos();
        
        // read out the data
        memcpy(dest, _payloadStart + currentPosition, numBytesToRead);
        
        // seek to the end of the read
        seek(currentPosition + numBytesToRead);
    }
    
    return numBytesToRead;
}
