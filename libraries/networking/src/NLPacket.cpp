//
//  NLPacket.cpp
//  libraries/networking/src
//
//  Created by Clement on 7/6/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NLPacket.h"

int64_t NLPacket::localHeaderSize(PacketType::Value type) {
    int64_t size = ((NON_SOURCED_PACKETS.contains(type)) ? 0 : NUM_BYTES_RFC4122_UUID) +
                    ((NON_VERIFIED_PACKETS.contains(type)) ? 0 : NUM_BYTES_RFC4122_UUID);
    return size;
}

int64_t NLPacket::maxPayloadSize(PacketType::Value type) {
    return Packet::maxPayloadSize(type) - localHeaderSize(type);
}

qint64 Packet::totalHeadersSize() const {
    return localHeaderSize() + Packet::localHeaderSize();
}

qint64 Packet::localHeaderSize() const {
    return localHeaderSize(_type);
}

std::unique_ptr<NLPacket> NLPacket::create(PacketType::Value type, int64_t size) {
    if (size > maxPayloadSize(type)) {
        return std::unique_ptr<NLPacket>();
    }
    
    return std::unique_ptr<NLPacket>(new NLPacket(type, size));
}

NLPacket::NLPacket(PacketType::Value type, int64_t size) : Packet(type, localHeaderSize(type) + size) {
}

void NLPacket::setSourceUuid(QUuid sourceUuid) {
    Q_ASSERT(!NON_SOURCED_PACKETS.contains(_type));
    auto offset = Packet::totalHeadersSize();
    memcpy(_packet.get() + offset, sourceUuid.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
}

void NLPacket::setConnectionUuid(QUuid connectionUuid) {
    Q_ASSERT(!NON_VERIFIED_PACKETS.contains(_type));
    auto offset = Packet::totalHeadersSize() +
                    ((NON_SOURCED_PACKETS.contains(_type)) ? 0 : NUM_BYTES_RFC4122_UUID);
    memcpy(_packet.get() + offset, connectionUuid.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
}