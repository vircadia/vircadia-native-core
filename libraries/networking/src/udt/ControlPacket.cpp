//
//  ControlPacket.cpp
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-24.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ControlPacket.h"

using namespace udt;

std::unique_ptr<ControlPacket> ControlPacket::create(Type type, qint64 size) {
    
    std::unique_ptr<ControlPacket> controlPacket;
    
    if (size == -1) {
        return ControlPacket::create(type);
    } else {
        // Fail with invalid size
        Q_ASSERT(size >= 0);
        
        return ControlPacket::create(type, size);
    }
}

ControlPacket::ControlPacketPair ControlPacket::createPacketPair(quint64 timestamp) {
    // create each of the two packets in the packet pair
    ControlPacketPair packetPair { std::unique_ptr<ControlPacket>(new ControlPacket(timestamp)),
                                   std::unique_ptr<ControlPacket>(new ControlPacket(timestamp)) };
    return packetPair;
}

qint64 ControlPacket::localHeaderSize() {
    return sizeof(ControlBitAndType);
}

qint64 ControlPacket::totalHeadersSize() const {
    return BasePacket::totalHeadersSize() + localHeaderSize();
}

ControlPacket::ControlPacket(Type type) :
    BasePacket(-1),
    _type(type)
{
    adjustPayloadStartAndCapacity();
    
    open(QIODevice::ReadWrite);
    
    writeControlBitAndType();
}

ControlPacket::ControlPacket(Type type, qint64 size) :
    BasePacket(localHeaderSize() + size),
    _type(type)
{
    adjustPayloadStartAndCapacity();
    
    open(QIODevice::ReadWrite);
    
    writeControlBitAndType();
}

ControlPacket::ControlPacket(quint64 timestamp) :
    BasePacket(localHeaderSize() + sizeof(timestamp)),
    _type(Type::PacketPair)
{
    adjustPayloadStartAndCapacity();
    
    open(QIODevice::ReadWrite);
    
    writeControlBitAndType();
    
    // pack in the timestamp
    writePrimitive(timestamp);
}

ControlPacket::ControlPacket(ControlPacket&& other) :
	BasePacket(std::move(other))
{
    _type = other._type;
}

ControlPacket& ControlPacket::operator=(ControlPacket&& other) {
    BasePacket::operator=(std::move(other));
    
    _type = other._type;
    
    return *this;
}

static const uint32_t CONTROL_BIT_MASK = 1 << (sizeof(ControlPacket::ControlBitAndType) - 1);

void ControlPacket::setType(udt::ControlPacket::Type type) {
    _type = type;
    
    writeType();
}

void ControlPacket::writeControlBitAndType() {
    ControlBitAndType* bitAndType = reinterpret_cast<ControlBitAndType*>(_packet.get());

    // write the control bit by OR'ing the current value with the CONTROL_BIT_MASK
    *bitAndType = (*bitAndType | CONTROL_BIT_MASK);
    
    writeType();
}

void ControlPacket::writeType() {
    ControlBitAndType* bitAndType = reinterpret_cast<ControlBitAndType*>(_packet.get());
    
    // write the type by OR'ing the new type with the current value & CONTROL_BIT_MASK
    *bitAndType = (*bitAndType & CONTROL_BIT_MASK) | (_type << sizeof((ControlPacket::Type) - 1));
}
