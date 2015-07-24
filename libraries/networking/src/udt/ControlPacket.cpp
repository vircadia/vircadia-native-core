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

qint64 ControlPacket::localHeaderSize() {
    return sizeof(TypeAndSubSequenceNumber);
}

qint64 ControlPacket::totalHeadersSize() const {
    return BasePacket::localHeaderSize() + localHeaderSize();
}

ControlPacket::ControlPacket(Type type, const SequenceNumberList& sequenceNumbers) :
    BasePacket(localHeaderSize() + (sizeof(Packet::SequenceNumber) * sequenceNumbers.size()))
{
    qint64 headerSize = localHeaderSize();
    
    _payloadCapacity -= headerSize;
    _payloadStart += headerSize;
    
    open(QIODevice::ReadWrite);
    
    // pack in the sequence numbers
    for(auto& sequenceNumber : sequenceNumbers) {
        writePrimitive(sequenceNumber);
    }
}

ControlPacket::ControlPacket(ControlPacket&& other) :
	BasePacket(std::move(other))
{
    
}

ControlPacket& ControlPacket::operator=(Packet&& other) {
    BasePacket::operator=(std::move(other));
    
    return *this;
}
