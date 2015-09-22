//
//  PacketQueue.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 9/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PacketQueue.h"

#include "Packet.h"
#include "PacketList.h"

using namespace udt;


MessageNumber PacketQueue::getNextMessageNumber() {
    static const MessageNumber MAX_MESSAGE_NUMBER = MessageNumber(1) << MESSAGE_NUMBER_BITS;
    _currentMessageNumber = (_currentMessageNumber + 1) % MAX_MESSAGE_NUMBER;
    return _currentMessageNumber;
}

bool PacketQueue::isEmpty() const {
    LockGuard locker(_packetsLock);
    return _packets.empty();
}

PacketQueue::PacketPointer PacketQueue::takeFront() {
    LockGuard locker(_packetsLock);
    if (!_packets.empty()) {
        auto packet = std::move(_packets.front());
        _packets.pop_front();
        return std::move(packet);
    }
    
    return PacketPointer();
}

void PacketQueue::queuePacket(PacketPointer packet) {
    LockGuard locker(_packetsLock);
    _packets.push_back(std::move(packet));
}

void PacketQueue::queuePacketList(PacketListPointer packetList) {
    Q_ASSERT(packetList->_packets.size() > 0);
    
    auto messageNumber = getNextMessageNumber();
    auto markPacket = [&messageNumber](const PacketPointer& packet, Packet::PacketPosition position) {
        packet->setPacketPosition(position);
        packet->writeMessageNumber(messageNumber);
    };
    
    if (packetList->_packets.size() == 1) {
        markPacket(packetList->_packets.front(), Packet::PacketPosition::ONLY);
    } else {
        const auto second = ++packetList->_packets.begin();
        const auto last = --packetList->_packets.end();
        std::for_each(second, last, [&](const PacketPointer& packet) {
            markPacket(packet, Packet::PacketPosition::MIDDLE);
        });
        
        markPacket(packetList->_packets.front(), Packet::PacketPosition::FIRST);
        markPacket(packetList->_packets.back(), Packet::PacketPosition::LAST);
    }
    
    LockGuard locker(_packetsLock);
    _packets.splice(_packets.end(), packetList->_packets);
}