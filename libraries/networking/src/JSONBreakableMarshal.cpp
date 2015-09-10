//
//  JSONBreakableMarshal.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 04/28/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "JSONBreakableMarshal.h"

#include <QtCore/QDebug>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

QVariantMap JSONBreakableMarshal::_interpolationMap = QVariantMap();

QByteArray JSONBreakableMarshal::toByteArray(const QJsonObject& jsonObject) {
    return QJsonDocument(jsonObject).toBinaryData();
}

QJsonObject JSONBreakableMarshal::fromByteArray(const QByteArray& buffer) {
    auto document = QJsonDocument::fromBinaryData(buffer);
    Q_ASSERT(document.isObject());
    auto object = document.object();
    
    QStringList currentKey;
    for (auto key : object.keys()) {
        interpolate(object[key], key);
    }
    
    return object;
}

void JSONBreakableMarshal::addInterpolationForKey(const QString& rootKey, const QString& interpolationKey,
                                                  const QString& interpolationValue) {
    // if there is no map already beneath this key in our _interpolationMap create a QVariantMap there now
    auto it = _interpolationMap.find(rootKey);
    if (it == _interpolationMap.end()) {
        it = _interpolationMap.insert(rootKey, QVariantMap());
    }
    Q_ASSERT(it->canConvert(QMetaType::QVariantMap));
    static_cast<QVariantMap*>(it->data())->insert(interpolationKey, interpolationValue);
}

void JSONBreakableMarshal::removeInterpolationForKey(const QString& rootKey, const QString& interpolationKey) {
    // make sure the interpolation map contains this root key and that the value is a map
    auto it = _interpolationMap.find(rootKey);
    if (it != _interpolationMap.end() && !it->isNull()) {
        // remove the value at the interpolationKey
        Q_ASSERT(it->canConvert(QMetaType::QVariantMap));
        static_cast<QVariantMap*>(it->data())->remove(interpolationKey);
    }
}

void JSONBreakableMarshal::interpolate(QJsonValueRef currentValue, QString lastKey) {
    if (currentValue.isArray()) {
        auto array = currentValue.toArray();
        for (int i = 0; i < array.size(); ++i) {
            // pass last key and recurse array
            interpolate(array[i], QString::number(i));
        }
        currentValue = array;
    } else if (currentValue.isObject()) {
        auto object = currentValue.toObject();
        for (auto key : object.keys()) {
            // pass last key and recurse object
            interpolate(object[key], key);
        }
        currentValue = object;
    } else if (currentValue.isString()) {
        // Maybe interpolate
        auto mapIt = _interpolationMap.find(lastKey);
        if (mapIt != _interpolationMap.end()) {
            Q_ASSERT(mapIt->canConvert(QMetaType::QVariantMap));
            auto interpolationMap = mapIt->toMap();
            
            auto result = interpolationMap.find(currentValue.toString());
            if (result != interpolationMap.end()) {
                // Replace value
                currentValue = result->toString();
            }
        }
    }
}
