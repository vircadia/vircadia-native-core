//
//  NetworkPacket.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  A really simple class that stores a network packet between being received and being processed
//

#ifndef __shared_NetworkPacket__
#define __shared_NetworkPacket__

#include <stdlib.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include "HifiSockAddr.h"

#include "NodeList.h" // for MAX_PACKET_SIZE

/// Storage of not-yet processed inbound, or not yet sent outbound generic UDP network packet
class NetworkPacket {
public:
    NetworkPacket();
    NetworkPacket(const NetworkPacket& packet); // copy constructor
    ~NetworkPacket(); // destructor
    NetworkPacket& operator= (const NetworkPacket& other);    // copy assignment

#ifdef HAS_MOVE_SEMANTICS
    NetworkPacket(NetworkPacket&& packet); // move?? // same as copy, but other packet won't be used further
    NetworkPacket& operator= (NetworkPacket&& other);         // move assignment
#endif

    NetworkPacket(const HifiSockAddr& sockAddr, unsigned char*  packetData, ssize_t packetLength);

    const HifiSockAddr& getSockAddr() const { return _sockAddr; }
    ssize_t getLength() const { return _packetLength; }
    unsigned char* getData() { return &_packetData[0]; }

    const HifiSockAddr& getAddress() const { return _sockAddr; }
    const unsigned char* getData() const { return &_packetData[0]; }

private:
    void copyContents(const HifiSockAddr& sockAddr, const unsigned char*  packetData, ssize_t packetLength);
    
    HifiSockAddr _sockAddr;
    ssize_t _packetLength;
    unsigned char _packetData[MAX_PACKET_SIZE];
};

#endif /* defined(__shared_NetworkPacket__) */
