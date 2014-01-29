//
//  NetworkPacket.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  A really simple class that stores a network packet between being received and being processed
//

#include <cstring>
#include <QtDebug>
#include <cassert>

#include "NetworkPacket.h"

void NetworkPacket::copyContents(const HifiSockAddr& sockAddr, const QByteArray& packet) {
    if (packet.size() && packet.size() <= MAX_PACKET_SIZE) {
        _sockAddr = sockAddr;
        _byteArray = packet;
    } else {
        qDebug(">>> NetworkPacket::copyContents() unexpected length = %d", packet.size());
    }
}

NetworkPacket::NetworkPacket(const NetworkPacket& packet) {
    copyContents(packet.getSockAddr(), packet.getByteArray());
}

NetworkPacket::NetworkPacket(const HifiSockAddr& sockAddr, const QByteArray& packet) {
    copyContents(sockAddr, packet);
};

// copy assignment 
NetworkPacket& NetworkPacket::operator=(NetworkPacket const& other) {
    copyContents(other.getSockAddr(), other.getByteArray());
    return *this;
}

#ifdef HAS_MOVE_SEMANTICS
// move, same as copy, but other packet won't be used further
NetworkPacket::NetworkPacket(NetworkPacket && packet) {
    copyContents(packet.getAddress(), packet.getByteArray());
}

// move assignment
NetworkPacket& NetworkPacket::operator=(NetworkPacket&& other) {
    copyContents(other.getAddress(), other.getByteArray());
    return *this;
}
#endif