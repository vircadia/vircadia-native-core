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

#include "Constants.h"

using namespace udt;

int ControlPacket::localHeaderSize() {
    return sizeof(ControlPacket::ControlBitAndType);
}
int ControlPacket::totalHeaderSize() {
    return BasePacket::totalHeaderSize() + ControlPacket::localHeaderSize();
}
int ControlPacket::maxPayloadSize() {
    return BasePacket::maxPayloadSize() - ControlPacket::localHeaderSize();
}

std::unique_ptr<ControlPacket> ControlPacket::fromReceivedPacket(std::unique_ptr<char[]> data, qint64 size,
                                                                 const HifiSockAddr &senderSockAddr) {
    // Fail with null data
    Q_ASSERT(data);
    
    // Fail with invalid size
    Q_ASSERT(size >= 0);
    
    // allocate memory
    auto packet = std::unique_ptr<ControlPacket>(new ControlPacket(std::move(data), size, senderSockAddr));
    
    packet->open(QIODevice::ReadOnly);
    
    return packet;
}

std::unique_ptr<ControlPacket> ControlPacket::create(Type type, qint64 size) {
    return std::unique_ptr<ControlPacket>(new ControlPacket(type, size));
}

ControlPacket::ControlPacket(Type type, qint64 size) :
    BasePacket((size == -1) ? -1 : ControlPacket::localHeaderSize() + size),
    _type(type)
{
    adjustPayloadStartAndCapacity(ControlPacket::localHeaderSize());
    
    open(QIODevice::ReadWrite);
    
    writeType();
}

ControlPacket::ControlPacket(std::unique_ptr<char[]> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    BasePacket(std::move(data), size, senderSockAddr)
{
    // sanity check before we decrease the payloadSize with the payloadCapacity
    Q_ASSERT(_payloadSize == _payloadCapacity);
    
    adjustPayloadStartAndCapacity(ControlPacket::localHeaderSize(), _payloadSize > 0);
    
    readType();
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

void ControlPacket::setType(udt::ControlPacket::Type type) {
    _type = type;
    
    writeType();
}

void ControlPacket::writeType() {
    ControlBitAndType* bitAndType = reinterpret_cast<ControlBitAndType*>(_packet.get());
    
    // We override the control bit here by writing the type but it's okay, it'll always be 1
    *bitAndType = CONTROL_BIT_MASK | (ControlBitAndType(_type) << (8 * sizeof(Type)));
}

void ControlPacket::readType() {
    ControlBitAndType bitAndType = *reinterpret_cast<ControlBitAndType*>(_packet.get());
    
    Q_ASSERT_X(bitAndType & CONTROL_BIT_MASK, "ControlPacket::readHeader()", "This should be a control packet");
    
    uint16_t packetType = (bitAndType & ~CONTROL_BIT_MASK) >> (8 * sizeof(Type));
    Q_ASSERT_X(packetType <= ControlPacket::Type::HandshakeRequest, "ControlPacket::readType()", "Received a control packet with wrong type");
    
    // read the type
    _type = (Type) packetType;
}
