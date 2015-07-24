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

std::unique_ptr<ControlPacket> ControlPacket::create(Type type, const SequenceNumberList& sequenceNumbers) {
    return ControlPacket::create(type, sequenceNumbers);
}

ControlPacket::ControlPacketPair ControlPacket::createPacketPair(quint64 timestamp) {
    // create each of the two packets in the packet pair
    ControlPacketPair packetPair { std::unique_ptr<ControlPacket>(new ControlPacket(timestamp)),
                                   std::unique_ptr<ControlPacket>(new ControlPacket(timestamp)) };
    return packetPair;
}

qint64 ControlPacket::localHeaderSize() {
    return sizeof(TypeAndSubSequenceNumber);
}

qint64 ControlPacket::totalHeadersSize() const {
    return BasePacket::localHeaderSize() + localHeaderSize();
}

ControlPacket::ControlPacket(Type type, const SequenceNumberList& sequenceNumbers) :
    BasePacket(localHeaderSize() + (sizeof(Packet::SequenceNumber) * sequenceNumbers.size())),
    _type(type)
{
    adjustPayloadStartAndCapacity();
    
    open(QIODevice::ReadWrite);
    
    // pack in the sequence numbers
    for (auto& sequenceNumber : sequenceNumbers) {
        writePrimitive(sequenceNumber);
    }
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
