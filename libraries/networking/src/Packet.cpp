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

int64_t Packet::headerSize(PacketType::Value type) {
    int64_t size = numBytesForArithmeticCodedPacketType(type) + sizeof(PacketVersion) +
                        ((SEQUENCE_NUMBERED_PACKETS.contains(type)) ? sizeof(SequenceNumber) : 0);
    return size;
}

int64_t Packet::maxPayloadSize(PacketType::Value type) {
    return MAX_PACKET_SIZE - headerSize(type);
}

std::unique_ptr<Packet> Packet::create(PacketType::Value type, int64_t size) {
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

Packet::Packet(PacketType::Value type, int64_t size) :
    _packetSize(headerSize(type) + size),
    _packet(new char(_packetSize)),
    _payload(_packet.get() + headerSize(type), size) {
        
        // Sanity check
        Q_ASSERT(size <= maxPayloadSize(type));
        
        // copy packet type and version in header
        setPacketTypeAndVersion(type);
        
        // Set control bit and sequence number to 0 if necessary
        if (SEQUENCE_NUMBERED_PACKETS.contains(type)) {
            setSequenceNumber(0);
        }
}

PacketType::Value Packet::getPacketType() const {
    return *reinterpret_cast<PacketType::Value*>(_packet.get());
}

PacketVersion Packet::getPacketTypeVersion() const {
    return *reinterpret_cast<PacketVersion*>(_packet.get() + numBytesForArithmeticCodedPacketType(getPacketType()));
}

Packet::SequenceNumber Packet::getSequenceNumber() const {
    PacketType::Value type{ getPacketType() };
    if (SEQUENCE_NUMBERED_PACKETS.contains(type)) {
        SequenceNumber seqNum = *reinterpret_cast<SequenceNumber*>(_packet.get() +
                                                                   numBytesForArithmeticCodedPacketType(type) +
                                                                   sizeof(PacketVersion));
        return seqNum & ~(1 << 15); // remove control bit
    }
    return -1;
}

bool Packet::isControlPacket() const {
    auto type = getPacketType();
    if (SEQUENCE_NUMBERED_PACKETS.contains(type)) {
        SequenceNumber seqNum = *reinterpret_cast<SequenceNumber*>(_packet.get() +
                                                                   numBytesForArithmeticCodedPacketType(type) +
                                                                   sizeof(PacketVersion));
        return seqNum & (1 << 15); // Only keep control bit
    }
    return false;
}

void Packet::setPacketTypeAndVersion(PacketType::Value type) {
    // Pack the packet type
    auto offset = packArithmeticallyCodedValue((int)type, _packet.get());
    
    // Pack the packet version
    auto version { versionForPacketType(type) };
    memcpy(_packet.get() + offset, &version, sizeof(version));
}

void Packet::setSequenceNumber(SequenceNumber seqNum) {
    auto type = getPacketType();
    // Here we are overriding the control bit to 0.
    // But that is not an issue since we should only ever set the seqNum
    // for data packets going out
    memcpy(_packet.get() + numBytesForArithmeticCodedPacketType(type) + sizeof(PacketVersion),
           &seqNum, sizeof(seqNum));
}
