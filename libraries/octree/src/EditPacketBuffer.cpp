//
//  EditPacketBuffer.cpp
//  libraries/octree/src
//
//  Created by Stephen Birarda on 2014-07-30.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EditPacketBuffer.h"

EditPacketBuffer::EditPacketBuffer() :
    _nodeUUID(),
    _currentType(PacketTypeUnknown),
    _currentSize(0),
    _satoshiCost(0)
{
    
}

EditPacketBuffer::EditPacketBuffer (PacketType::Value type, unsigned char* buffer, size_t length, qint64 satoshiCost, QUuid nodeUUID) :
    _nodeUUID(nodeUUID),
    _currentType(type),
    _currentSize(length),
    _satoshiCost(satoshiCost)
{
    memcpy(_currentBuffer, buffer, length);
}