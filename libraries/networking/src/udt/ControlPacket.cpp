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
        controlPacket = ControlPacket::create(type);
    } else {
        // Fail with invalid size
        Q_ASSERT(size >= 0);
        
        controlPacket = ControlPacket::create(type, size);
    }
    
    controlPacket->open(QIODevice::ReadWrite);
    
    return controlPacket;
}

ControlPacket::ControlPacketPair ControlPacket::createPacketPair(quint64 timestamp) {
    // create each of the two packets in the packet pair
    ControlPacketPair packetPair { std::unique_ptr<ControlPacket>(new ControlPacket(timestamp)),
                                   std::unique_ptr<ControlPacket>(new ControlPacket(timestamp)) };
    return packetPair;
}

qint64 ControlPacket::localHeaderSize() {
    return sizeof(BitFieldAndControlType);
}

qint64 ControlPacket::totalHeadersSize() const {
    return BasePacket::totalHeadersSize() + localHeaderSize();
}

ControlPacket::ControlPacket(Type type) :
    BasePacket(-1),
    _type(type)
{
    adjustPayloadStartAndCapacity();
}

ControlPacket::ControlPacket(Type type, qint64 size) :
    BasePacket(localHeaderSize() + size),
    _type(type)
{
    adjustPayloadStartAndCapacity();
}

ControlPacket::ControlPacket(quint64 timestamp) :
    BasePacket(localHeaderSize() + sizeof(timestamp)),
    _type(Type::PacketPair)
{
    adjustPayloadStartAndCapacity();
    
    open(QIODevice::ReadWrite);
    
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
