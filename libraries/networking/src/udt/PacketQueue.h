//
//  PacketQueue.h
//  libraries/networking/src/udt
//
//  Created by Clement on 9/16/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketQueue_h
#define hifi_PacketQueue_h

#include <list>
#include <vector>
#include <memory>
#include <mutex>

#include "Packet.h"

namespace udt {
    
class PacketList;
    
using MessageNumber = uint32_t;
    
class PacketQueue {
    using Mutex = std::recursive_mutex;
    using LockGuard = std::lock_guard<Mutex>;
    using PacketPointer = std::unique_ptr<Packet>;
    using PacketListPointer = std::unique_ptr<PacketList>;
    using Channel = std::unique_ptr<std::list<PacketPointer>>;
    using Channels = std::vector<Channel>;
    
public:
    PacketQueue();
    void queuePacket(PacketPointer packet);
    void queuePacketList(PacketListPointer packetList);
    
    bool isEmpty() const;
    PacketPointer takePacket();
    
    Mutex& getLock() { return _packetsLock; }
    
private:
    MessageNumber getNextMessageNumber();
    unsigned int nextIndex();
    
    MessageNumber _currentMessageNumber { 0 };
    
    mutable Mutex _packetsLock; // Protects the packets to be sent.
    Channels _channels; // One channel per packet list + Main channel
    unsigned int _currentIndex { 0 };
};

}


#endif // hifi_PacketQueue_h