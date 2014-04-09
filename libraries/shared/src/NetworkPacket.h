//
//  NetworkPacket.h
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 8/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  A really simple class that stores a network packet between being received and being processed
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __shared_NetworkPacket__
#define __shared_NetworkPacket__

#include <stdlib.h>

#ifndef _WIN32
#include <arpa/inet.h> // not available on windows
#include <ifaddrs.h>
#endif

#include "NodeList.h"

/// Storage of not-yet processed inbound, or not yet sent outbound generic UDP network packet
class NetworkPacket {
public:
    NetworkPacket(const NetworkPacket& packet); // copy constructor
    NetworkPacket& operator= (const NetworkPacket& other);    // copy assignment

#ifdef HAS_MOVE_SEMANTICS
    NetworkPacket(NetworkPacket&& packet); // move?? // same as copy, but other packet won't be used further
    NetworkPacket& operator= (NetworkPacket&& other);         // move assignment
#endif

    NetworkPacket(const SharedNodePointer& destinationNode, const QByteArray& byteArray);

    const SharedNodePointer& getDestinationNode() const { return _destinationNode; }
    const QByteArray& getByteArray() const { return _byteArray; }

private:
    void copyContents(const SharedNodePointer& destinationNode, const QByteArray& byteArray);

    SharedNodePointer _destinationNode;
    QByteArray _byteArray;
};

#endif /* defined(__shared_NetworkPacket__) */
