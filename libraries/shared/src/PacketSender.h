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

/// Generalized threaded processor for queueing and sending of outbound packets. 
class PacketSender : public GenericThread {
public:

    PacketSender();

    /// Add packet to outbound queue.
    /// \param sockaddr& address the destination address
    /// \param packetData pointer to data
    /// \param ssize_t packetLength size of data
    /// \thread any thread, typically the application thread
    void queuePacket(sockaddr& address, unsigned char*  packetData, ssize_t packetLength);
    
private:
    virtual bool process();

    std::vector<NetworkPacket> _packets;
    uint64_t _lastSendTime;
    
};

#endif // __shared__PacketSender__
