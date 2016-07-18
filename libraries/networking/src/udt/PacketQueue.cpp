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

#include "PacketList.h"

using namespace udt;

MessageNumber PacketQueue::getNextMessageNumber() {
    static const MessageNumber MAX_MESSAGE_NUMBER = MessageNumber(1) << MESSAGE_NUMBER_SIZE;
    _currentMessageNumber = (_currentMessageNumber + 1) % MAX_MESSAGE_NUMBER;
    return _currentMessageNumber;
}

bool PacketQueue::isEmpty() const {
    LockGuard locker(_packetsLock);
    // Only the main channel and it is empty
    return (_channels.size() == 1) && _channels.front().empty();
}

PacketQueue::PacketPointer PacketQueue::takePacket() {
    LockGuard locker(_packetsLock);
    if (isEmpty()) {
        return PacketPointer();
    }
    
    // Find next non empty channel
    if (_channels[nextIndex()].empty()) {
        nextIndex();
    }
    auto& channel = _channels[_currentIndex];
    Q_ASSERT(!channel.empty());
    
    // Take front packet
    auto packet = std::move(channel.front());
    channel.pop_front();
    
    // Remove now empty channel (Don't remove the main channel)
    if (channel.empty() && _currentIndex != 0) {
        channel.swap(_channels.back());
        _channels.pop_back();
        --_currentIndex;
    }
    
    return packet;
}

unsigned int PacketQueue::nextIndex() {
    _currentIndex = (_currentIndex + 1) % _channels.size();
    return _currentIndex;
}

void PacketQueue::queuePacket(PacketPointer packet) {
    LockGuard locker(_packetsLock);
    _channels.front().push_back(std::move(packet));
}

void PacketQueue::queuePacketList(PacketListPointer packetList) {
    if (packetList->isOrdered()) {
        packetList->preparePackets(getNextMessageNumber());
    }
    
    LockGuard locker(_packetsLock);
    _channels.push_back(std::move(packetList->_packets));
}
