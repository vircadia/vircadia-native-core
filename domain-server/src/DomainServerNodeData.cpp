//
//  DomainServerNodeData.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2/6/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>

#include <JSONBreakableMarshal.h>
#include <udt/PacketHeaders.h>

#include "DomainServerNodeData.h"

DomainServerNodeData::DomainServerNodeData() :
    _sessionSecretHash(),
    _assignmentUUID(),
    _walletUUID(),
    _username(),
    _paymentIntervalTimer(),
    _statsJSONObject(),
    _sendingSockAddr(),
    _isAuthenticated(true)
{
    _paymentIntervalTimer.start();
}

void DomainServerNodeData::processJSONStatsPacket(NLPacket& statsPacket) {
    QVariantMap packetVariantMap = JSONBreakableMarshal::fromStringBuffer(statsPacket.readAll());
    _statsJSONObject = mergeJSONStatsFromNewObject(QJsonObject::fromVariantMap(packetVariantMap), _statsJSONObject);
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
