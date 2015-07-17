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

void NetworkPacket::copyContents(const SharedNodePointer& node, const QByteArray& packet) {
    if (packet.size() && packet.size() <= MAX_PACKET_SIZE) {
        _node = node;
        _byteArray = packet;
    } else {
        qCDebug(networking, ">>> NetworkPacket::copyContents() unexpected length = %d", packet.size());
    }
}

NetworkPacket::NetworkPacket(const NetworkPacket& packet) {
    copyContents(packet.getNode(), packet.getByteArray());
}

NetworkPacket::NetworkPacket(const SharedNodePointer& node, const QByteArray& packet) {
    copyContents(node, packet);
};

// copy assignment 
NetworkPacket& NetworkPacket::operator=(NetworkPacket const& other) {
    copyContents(other.getNode(), other.getByteArray());
    return *this;
}
