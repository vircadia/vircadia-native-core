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

NetworkPacket::NetworkPacket(const NetworkPacket& other) :
        _node(other._node),
        _nlPacket(other._nlPacket) {
}

NetworkPacket::NetworkPacket(const SharedNodePointer& node, const NLPacket& packet) {
    if (packet.getSizeWitHeader() && packet.getSizeWithHeader() <= MAX_nlPacket_SIZE) {
        _node = node;
        _nlPacket = packet;
    } else {
        qCDebug(networking, ">>> NetworkPacket::copyContents() unexpected length = %d", packet.size());
    }
};

// copy assignment 
NetworkPacket& NetworkPacket::operator=(NetworkPacket const& other) {
    _node = other._node;
    _nlPacket = other._nlPacket;
    return *this;
}

#ifdef HAS_MOVE_SEMANTICS
// move, same as copy, but other packet won't be used further
NetworkPacket::NetworkPacket(NetworkPacket&& other) :
        _node(std::move(other._node)),
        _nlPacket(std::move(other._nlPacket)) {
}

// move assignment
NetworkPacket& NetworkPacket::operator=(NetworkPacket&& other) {
    _node = std::move(other._node);
    _nlPacket = std::move(other._nlPacket);
    return *this;
}
#endif
