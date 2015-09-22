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
#include <memory>
#include <mutex>

namespace udt {
    
class Packet;
class PacketList;
    
using MessageNumber = uint32_t;
    
class PacketQueue {
    using Mutex = std::recursive_mutex;
    using LockGuard = std::lock_guard<Mutex>;
    using PacketPointer = std::unique_ptr<Packet>;
    using PacketListPointer = std::unique_ptr<PacketList>;
    using PacketList = std::list<PacketPointer>;
    
public:
    void queuePacket(PacketPointer packet);
    void queuePacketList(PacketListPointer packetList);
    
    bool isEmpty() const;
    PacketPointer takeFront();
    
    MessageNumber getNextMessageNumber();
    Mutex& getLock() { return _packetsLock; }
    
private:
    MessageNumber _currentMessageNumber { 0 };
    
    mutable Mutex _packetsLock; // Protects the packets to be sent list.
    PacketList _packets; // List of packets to be sent
};

}


#endif // hifi_PacketQueue_h