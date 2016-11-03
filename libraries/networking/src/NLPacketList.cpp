//
//  NLPacketList.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 07/06/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NLPacketList.h"

#include "udt/Packet.h"


std::unique_ptr<NLPacketList> NLPacketList::create(PacketType packetType, QByteArray extendedHeader,
                                                   bool isReliable, bool isOrdered) {
    auto nlPacketList = std::unique_ptr<NLPacketList>(new NLPacketList(packetType, extendedHeader,
                                                                       isReliable, isOrdered));
    nlPacketList->open(WriteOnly);
    return nlPacketList;
}

NLPacketList::NLPacketList(PacketType packetType, QByteArray extendedHeader, bool isReliable, bool isOrdered) :
    PacketList(packetType, extendedHeader, isReliable, isOrdered)
{
}

NLPacketList::NLPacketList(PacketList&& other) : PacketList(std::move(other)) {
    // Update _packets
    for (auto& packet : _packets) {
        packet = NLPacket::fromBase(std::move(packet));
    }

    if (_packets.size() > 0) {
        auto nlPacket = static_cast<const NLPacket*>(_packets.front().get());
        _sourceID = nlPacket->getSourceID();
        _packetType = nlPacket->getType();
        _packetVersion = nlPacket->getVersion();
    }
}

std::unique_ptr<udt::Packet> NLPacketList::createPacket() {
    return NLPacket::create(getType(), -1, isReliable(), isOrdered());
}
