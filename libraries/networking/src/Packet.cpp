//
//  Packet.cpp
//
//
//  Created by Clement on 7/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Packet.h"

int64_t Packet::headerSize() {
    return sizeof(Packet::Header);
}

int64_t Packet::maxPayloadSize() {
    return MAX_PACKET_SIZE - Packet::headerSize();
}

std::unique_ptr<Packet> Packet::create(PacketType::Value type, int64_t size) {
    if (size == -1) {
        size = maxPayloadSize();
    }
    if (size <= 0 || size > maxPayloadSize()) {
        return std::unique_ptr<Packet>();
    }
    
    return std::unique_ptr<Packet>(new Packet(type, size));
}

Packet::Packet(PacketType::Value type, int64_t size) {
    _packetSize = headerSize() + size;
    _packet = std::unique_ptr<char>(new char(_packetSize));
    _payloadStart = _packet.get() + headerSize();
}

const Packet::Header& Packet::getHeader() const {
    return *reinterpret_cast<const Header*>(_packet.get());
}

Packet::Header& Packet::getHeader() {
    return *reinterpret_cast<Header*>(_packet.get());
}

char* Packet::getPayload() {
    return _payloadStart;
}


int64_t NodeListPacket::headerSize() {
    return sizeof(NodeListPacket::Header);
}

int64_t NodeListPacket::maxPayloadSize() {
    return Packet::maxPayloadSize() - NodeListPacket::headerSize();
}

std::unique_ptr<NodeListPacket> NodeListPacket::create(PacketType::Value type, int64_t size) {
    if (size > maxPayloadSize()) {
        return std::unique_ptr<NodeListPacket>();
    }
    
    return std::unique_ptr<NodeListPacket>(new NodeListPacket(type, size));
}

NodeListPacket::NodeListPacket(PacketType::Value type, int64_t size) : Packet(type, headerSize() + size) {
    
}