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

NetworkPacket::NetworkPacket() {
    _packetLength = 0;
}

NetworkPacket::~NetworkPacket() {
    // nothing to do
}

void NetworkPacket::copyContents(const HifiSockAddr& sockAddr, const unsigned char*  packetData, ssize_t packetLength) {
    _packetLength = 0;
    if (packetLength >=0 && packetLength <= MAX_PACKET_SIZE) {
        _sockAddr = sockAddr;
        _packetLength = packetLength;
        memcpy(&_packetData[0], packetData, packetLength);
    } else {
        qDebug(">>> NetworkPacket::copyContents() unexpected length=%lu\n",packetLength);
    }
}

NetworkPacket::NetworkPacket(const NetworkPacket& packet) {
    copyContents(packet.getSockAddr(), packet.getData(), packet.getLength());
}

NetworkPacket::NetworkPacket(const HifiSockAddr& sockAddr, unsigned char*  packetData, ssize_t packetLength) {
    copyContents(sockAddr, packetData, packetLength);
};

// copy assignment 
NetworkPacket& NetworkPacket::operator=(NetworkPacket const& other) {
    copyContents(other.getSockAddr(), other.getData(), other.getLength());
    return *this;
}

#ifdef HAS_MOVE_SEMANTICS
// move, same as copy, but other packet won't be used further
NetworkPacket::NetworkPacket(NetworkPacket && packet) {
    copyContents(packet.getAddress(), packet.getData(), packet.getLength());
}

// move assignment
NetworkPacket& NetworkPacket::operator=(NetworkPacket&& other) {
    _packetLength = 0;
    copyContents(other.getAddress(), other.getData(), other.getLength());
    return *this;
}
#endif