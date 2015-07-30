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

std::unique_ptr<ControlPacket> ControlPacket::fromReceivedPacket(std::unique_ptr<char> data, qint64 size,
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
    
    std::unique_ptr<ControlPacket> controlPacket;
    
    if (size == -1) {
        return ControlPacket::create(type);
    } else {
        // Fail with invalid size
        Q_ASSERT(size >= 0);
        
        return ControlPacket::create(type, size);
    }
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
    adjustPayloadStartAndCapacity(localHeaderSize());
    
    open(QIODevice::ReadWrite);
    
    writeControlBitAndType();
}

ControlPacket::ControlPacket(Type type, qint64 size) :
    BasePacket(localHeaderSize() + size),
    _type(type)
{
    adjustPayloadStartAndCapacity(localHeaderSize());
    
    open(QIODevice::ReadWrite);
    
    writeControlBitAndType();
}

ControlPacket::ControlPacket(std::unique_ptr<char> data, qint64 size, const HifiSockAddr& senderSockAddr) :
    BasePacket(std::move(data), size, senderSockAddr)
{
    // sanity check before we decrease the payloadSize with the payloadCapacity
    Q_ASSERT(_payloadSize == _payloadCapacity);
    
    adjustPayloadStartAndCapacity(_payloadSize > 0);
    
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

void ControlPacket::writeControlBitAndType() {
    ControlBitAndType* bitAndType = reinterpret_cast<ControlBitAndType*>(_packet.get());

    // write the control bit by OR'ing the current value with the CONTROL_BIT_MASK
    *bitAndType = (*bitAndType | CONTROL_BIT_MASK);
    
    writeType();
}

void ControlPacket::writeType() {
    ControlBitAndType* bitAndType = reinterpret_cast<ControlBitAndType*>(_packet.get());
    
    // write the type by OR'ing the new type with the current value & CONTROL_BIT_MASK
    *bitAndType = (*bitAndType & CONTROL_BIT_MASK) | (_type << (sizeof(ControlPacket::Type) * 8 - 1));
}

void ControlPacket::readType() {
    ControlBitAndType bitAndType = *reinterpret_cast<ControlBitAndType*>(_packet.get());
    
    Q_ASSERT_X(bitAndType & CONTROL_BIT_MASK, "ControlPacket::readHeader()", "This should be a control packet");
    
    // read the type
    uint32_t oversizeType = (uint32_t) (bitAndType & ~CONTROL_BIT_MASK);
    _type = (Type) oversizeType;
}
