//
//  DomainServerNodeData.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/6/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDataStream>
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>

#include <PacketHeaders.h>

#include "DomainServerNodeData.h"

DomainServerNodeData::DomainServerNodeData() :
    _sessionSecretHash(),
    _staticAssignmentUUID(),
    _statsJSONObject()
{
    
}

void DomainServerNodeData::parseJSONStatsPacket(const QByteArray& statsPacket) {
    // push past the packet header
    QDataStream packetStream(statsPacket);
    packetStream.skipRawData(numBytesForPacketHeader(statsPacket));
    
    QVariantMap unpackedVariantMap;
    
    packetStream >> unpackedVariantMap;
    
    QJsonObject unpackedStatsJSON = QJsonObject::fromVariantMap(unpackedVariantMap);
    _statsJSONObject = mergeJSONStatsFromNewObject(unpackedStatsJSON, _statsJSONObject);
}


QJsonObject DomainServerNodeData::mergeJSONStatsFromNewObject(const QJsonObject& newObject, QJsonObject destinationObject) {
    foreach(const QString& key, newObject.keys()) {
        if (newObject[key].isObject() && destinationObject.contains(key)) {
            destinationObject[key] = mergeJSONStatsFromNewObject(newObject[key].toObject(), destinationObject[key].toObject());
        } else {
            destinationObject[key] = newObject[key];
        }
    }
    
    return destinationObject;
}