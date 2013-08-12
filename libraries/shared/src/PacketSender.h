//
//  PacketSender.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet sender.
//

#ifndef __shared__PacketSender__
#define __shared__PacketSender__

#include "GenericThread.h"
#include "NetworkPacket.h"

class PacketSender : public GenericThread {
public:

    PacketSender();
    // Call this when you have a packet you'd like sent...
    void queuePacket(sockaddr& address, unsigned char*  packetData, ssize_t packetLength);
    
    virtual bool process();
    
private:
    std::vector<NetworkPacket> _packets;
    uint64_t _lastSendTime;
    
};

#endif // __shared__PacketSender__
