//
//  NetworkPacket.cpp
//  libraries/networking/src
//
//  Created by Brad Hefta-Gaub on 8/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cassert>
#include <cstring>
#include <QtDebug>

#include "SharedUtil.h"
#include "NetworkLogging.h"

#include "NetworkPacket.h"

void NetworkPacket::copyContents(const SharedNodePointer& node, const NLPacket& packet) {
    if (packet.size() && packet.size() <= MAX_PACKET_SIZE) {
        _node = node;
        _nlPacket = packet;
    } else {
        qCDebug(networking, ">>> NetworkPacket::copyContents() unexpected length = %d", packet.size());
    }
}

NetworkPacket::NetworkPacket(const NetworkPacket& other) {
    copyContents(other._node, other._packet);
}

NetworkPacket::NetworkPacket(const SharedNodePointer& node, const NLPacket& packet) {
    copyContents(node, packet);
};

// copy assignment 
NetworkPacket& NetworkPacket::operator=(NetworkPacket const& other) {
    copyContents(other._node, other._packet);
    return *this;
}

#ifdef HAS_MOVE_SEMANTICS
// move, same as copy, but other packet won't be used further
NetworkPacket::NetworkPacket(NetworkPacket&& other) {
    copyContents(other._node, other._packet);
}

// move assignment
NetworkPacket& NetworkPacket::operator=(NetworkPacket&& other) {
    copyContents(other._node, other._packet);
    return *this;
}
#endif
