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
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QVariant>

#include <udt/PacketHeaders.h>

#include "DomainServerNodeData.h"

DomainServerNodeData::StringPairHash DomainServerNodeData::_overrideHash;

DomainServerNodeData::DomainServerNodeData() {
    _paymentIntervalTimer.start();
}

void DomainServerNodeData::updateJSONStats(QByteArray statsByteArray) {
    auto document = QJsonDocument::fromBinaryData(statsByteArray);
    Q_ASSERT(document.isObject());
    _statsJSONObject = overrideValuesIfNeeded(document.object());
}

QJsonObject DomainServerNodeData::overrideValuesIfNeeded(const QJsonObject& newStats) {
    QJsonObject result;
    for (auto it = newStats.constBegin(); it != newStats.constEnd(); ++it) {
        const auto& key = it.key();
        const auto& value = it.value();
        
        auto overrideIt = value.isString() ? _overrideHash.find({key, value.toString()}) : _overrideHash.end();
        if (overrideIt != _overrideHash.end()) {
            // We have a match, override the value
            result[key] = *overrideIt;
        } else if (value.isObject()) {
            result[key] = overrideValuesIfNeeded(value.toObject());
        } else if (value.isArray()) {
            result[key] = overrideValuesIfNeeded(value.toArray());
        } else {
            result[key] = newStats[key];
        }
    }
    return result;
}

QJsonArray DomainServerNodeData::overrideValuesIfNeeded(const QJsonArray& newStats) {
    QJsonArray result;
    for (const auto& value : newStats) {
        if (value.isObject()) {
            result.push_back(overrideValuesIfNeeded(value.toObject()));
        } else if (value.isArray()) {
            result.push_back(overrideValuesIfNeeded(value.toArray()));
        } else {
            result.push_back(value);
        }
    }
    return result;
}

void DomainServerNodeData::addOverrideForKey(const QString& key, const QString& value,
                                             const QString& overrideValue) {
    // Insert override value
    _overrideHash.insert({key, value}, overrideValue);
}

void DomainServerNodeData::removeOverrideForKey(const QString& key, const QString& value) {
    // Remove override value
    _overrideHash.remove({key, value});
}
