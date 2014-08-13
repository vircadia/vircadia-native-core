//
//  EntityEditPacketSender.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <PerfStat.h>
#include <OctalCode.h>
#include <PacketHeaders.h>
#include "EntityEditPacketSender.h"
#include "EntityItem.h"


void EntityEditPacketSender::adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew) {
    EntityItem::adjustEditPacketForClockSkew(codeColorBuffer, length, clockSkew);
}

void EntityEditPacketSender::queueEditEntityMessage(PacketType type, EntityItemID modelID, 
                                                                const EntityItemProperties& properties) {
    if (!_shouldSend) {
        return; // bail early
    }

    // use MAX_PACKET_SIZE since it's static and guaranteed to be larger than _maxPacketSize
    static unsigned char bufferOut[MAX_PACKET_SIZE];
    int sizeOut = 0;

    if (EntityItemProperties::encodeEntityEditPacket(type, modelID, properties, &bufferOut[0], _maxPacketSize, sizeOut)) {
        queueOctreeEditMessage(type, bufferOut, sizeOut);
    }
}

void EntityEditPacketSender::queueEraseEntityMessage(const EntityItemID& entityItemID) {
    if (!_shouldSend) {
        return; // bail early
    }

qDebug() << "EntityEditPacketSender::queueEraseEntityMessage() entityItemID=" << entityItemID;

    // use MAX_PACKET_SIZE since it's static and guaranteed to be larger than _maxPacketSize
    static unsigned char bufferOut[MAX_PACKET_SIZE];
    size_t sizeOut = 0;

qDebug() << "    _maxPacketSize=" << _maxPacketSize;

    if (EntityItemProperties::encodeEraseEntityMessage(entityItemID, &bufferOut[0], _maxPacketSize, sizeOut)) {

qDebug() << "   encodeEraseEntityMessage()... sizeOut=" << sizeOut;

        queueOctreeEditMessage(PacketTypeEntityErase, bufferOut, sizeOut);
    }
}
