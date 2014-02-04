//
//  MetavoxelUtil.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/30/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QByteArray>
#include <QtDebug>

#include <HifiSockAddr.h>
#include <PacketHeaders.h>

#include "MetavoxelUtil.h"

QUuid readSessionID(const QByteArray& data, const HifiSockAddr& sender, int& headerPlusIDSize) {
    // get the header size
    int headerSize = numBytesForPacketHeader(data);
    
    // read the session id
    const int UUID_BYTES = 16;
    headerPlusIDSize = headerSize + UUID_BYTES;
    if (data.size() < headerPlusIDSize) {
        qWarning() << "Metavoxel data too short [size=" << data.size() << ", sender=" << sender << "]\n";
        return QUuid();
    }
    return QUuid::fromRfc4122(QByteArray::fromRawData(data.constData() + headerSize, UUID_BYTES));
}

bool Box::contains(const Box& other) const {
    return other.minimum.x >= minimum.x && other.maximum.x <= maximum.x &&
        other.minimum.y >= minimum.y && other.maximum.y <= maximum.y &&
        other.minimum.z >= minimum.z && other.maximum.z <= maximum.z;
}
