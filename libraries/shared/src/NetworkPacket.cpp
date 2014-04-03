//
//  NetworkPacket.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  A really simple class that stores a network packet between being received and being processed
//

#include <cassert>
#include <cstring>
#include <QtDebug>

#include "SharedUtil.h"

#include "NetworkPacket.h"

void NetworkPacket::copyContents(const SharedNodePointer& destinationNode, const QByteArray& packet) {
    if (packet.size() && packet.size() <= MAX_PACKET_SIZE) {
        _destinationNode = destinationNode;
        _byteArray = packet;
    } else {
        qDebug(">>> NetworkPacket::copyContents() unexpected length = %d", packet.size());
    }
}

NetworkPacket::NetworkPacket(const NetworkPacket& packet) {
    copyContents(packet.getDestinationNode(), packet.getByteArray());
}

NetworkPacket::NetworkPacket(const SharedNodePointer& destinationNode, const QByteArray& packet) {
    copyContents(destinationNode, packet);
};

// copy assignment 
NetworkPacket& NetworkPacket::operator=(NetworkPacket const& other) {
    copyContents(other.getDestinationNode(), other.getByteArray());
    return *this;
}

#ifdef HAS_MOVE_SEMANTICS
// move, same as copy, but other packet won't be used further
NetworkPacket::NetworkPacket(NetworkPacket && packet) {
    copyContents(packet.getDestinationNode(), packet.getByteArray());
}

// move assignment
NetworkPacket& NetworkPacket::operator=(NetworkPacket&& other) {
    copyContents(other.getDestinationNode(), other.getByteArray());
    return *this;
}
#endif
