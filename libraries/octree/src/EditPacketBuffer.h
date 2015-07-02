//
//  EditPacketBuffer.h
//  libraries/octree/src
//
//  Created by Stephen Birarda on 2014-07-30.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EditPacketBuffer_h
#define hifi_EditPacketBuffer_h

#include <QtCore/QUuid>

#include <LimitedNodeList.h>
#include <PacketHeaders.h>

/// Used for construction of edit packets
class EditPacketBuffer {
public:
    EditPacketBuffer();
    EditPacketBuffer (PacketType::Value type, unsigned char* codeColorBuffer, size_t length,
                     qint64 satoshiCost = 0, const QUuid nodeUUID = QUuid());
    
    QUuid _nodeUUID;
    PacketType::Value _currentType;
    unsigned char _currentBuffer[MAX_PACKET_SIZE];
    size_t _currentSize;
    qint64 _satoshiCost;
};

#endif // hifi_EditPacketBuffer_h