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

PacketQueue::PacketQueue(MessageNumber messageNumber) : _currentMessageNumber(messageNumber) {
    _channels.emplace_front(new std::list<PacketPointer>());
    _currentChannel = _channels.begin();
}

MessageNumber PacketQueue::getNextMessageNumber() {
    static const MessageNumber MAX_MESSAGE_NUMBER = MessageNumber(1) << MESSAGE_NUMBER_SIZE;
    _currentMessageNumber = (_currentMessageNumber + 1) % MAX_MESSAGE_NUMBER;
    return _currentMessageNumber;
}

bool PacketQueue::isEmpty() const {
    LockGuard locker(_packetsLock);

    // Only the main channel and it is empty
    return _channels.size() == 1 && _channels.front()->empty();
}

PacketQueue::PacketPointer PacketQueue::takePacket() {
    LockGuard locker(_packetsLock);

    if (isEmpty()) {
        return PacketPointer();
    }

    // handle the case where we are looking at the first channel and it is empty
    if (_currentChannel == _channels.begin() && (*_currentChannel)->empty()) {
        ++_currentChannel;
    }

    // at this point the current channel should always not be at the end and should also not be empty
    Q_ASSERT(_currentChannel != _channels.end());

    auto& channel = *_currentChannel;

    Q_ASSERT(!channel->empty());

    // Take front packet
    auto packet = std::move(channel->front());
    channel->pop_front();

    // Remove now empty channel (Don't remove the main channel)
    if (channel->empty() && _currentChannel != _channels.begin()) {
        // erase the current channel and slide the iterator to the next channel
        _currentChannel = _channels.erase(_currentChannel);
    } else {
        ++_currentChannel;
    }

    // push forward our number of channels taken from
    ++_channelsVisitedCount;

    // check if we need to restart back at the front channel (main)
    // to respect our capped number of channels considered concurrently
    static const int MAX_CHANNELS_SENT_CONCURRENTLY = 16;

    if (_currentChannel == _channels.end() || _channelsVisitedCount >= MAX_CHANNELS_SENT_CONCURRENTLY) {
        _channelsVisitedCount = 0;
        _currentChannel = _channels.begin();
    }

    return packet;
}

void PacketQueue::queuePacket(PacketPointer packet) {
    LockGuard locker(_packetsLock);
    _channels.front()->push_back(std::move(packet));
}

void PacketQueue::queuePacketList(PacketListPointer packetList) {
    if (packetList->isOrdered()) {
        packetList->preparePackets(getNextMessageNumber());
    }

    LockGuard locker(_packetsLock);
    _channels.emplace_back(new std::list<PacketPointer>());
    _channels.back()->swap(packetList->_packets);
}
