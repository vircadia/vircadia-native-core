//
//  PacketReceiver.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet receiver.
//

#ifndef __shared__PacketReceiver__
#define __shared__PacketReceiver__

#include "GenericThread.h"
#include "NetworkPacket.h"

class PacketReceiver : public GenericThread {
public:

    // Call this when your network receive gets a packet
    void queuePacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);
    
    // implement this to process the incoming packets
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength) = 0;

    virtual bool process();
    
private:
    std::vector<NetworkPacket> _packets;
};

extern "C" void* PacketReceiverThreadEntry(void* arg);

#endif // __shared__PacketReceiver__
