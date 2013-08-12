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

#include "NodeList.h" // for MAX_PACKET_SIZE

class NetworkPacket {
public:
    NetworkPacket(sockaddr& address, unsigned char*  packetData, ssize_t packetLength);
    NetworkPacket(const NetworkPacket& packet);
    NetworkPacket();
    
    sockaddr&      getAddress()       { return _address; };
    ssize_t        getLength() const  { return _packetLength;  };
    unsigned char* getData()          { return &_packetData[0]; };

    const sockaddr&      getAddress() const { return _address; };
    const unsigned char* getData() const    { return &_packetData[0]; };

private:
    sockaddr        _address;
    ssize_t         _packetLength;
    unsigned char   _packetData[MAX_PACKET_SIZE];
};

#endif /* defined(__shared_NetworkPacket__) */
