//
//  Created by Bradley Austin Davis on 2015/11/09
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "JSONHelpers.h"

#include <QtCore/QJsonValue>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

template <typename T> 
QJsonValue glmToJson(const T& t) {
    static const T DEFAULT_VALUE = T();
    if (t == DEFAULT_VALUE) {
        return QJsonValue();
    }
    QJsonArray result;
    for (auto i = 0; i < t.length(); ++i) {
        result.push_back(t[i]);
    }
    return result;
}

template <typename T> 
T glmFromJson(const QJsonValue& json) {
    static const T DEFAULT_VALUE = T();
    T result;
    if (json.isArray()) {
        QJsonArray array = json.toArray();
        size_t length = std::min(array.size(), result.length());
        for (size_t i = 0; i < length; ++i) {
            result[i] = (float)array[i].toDouble();
        }
    }
    return result;
}

QJsonValue toJsonValue(const quat& q) {
    return glmToJson(q);
}

QJsonValue toJsonValue(const vec3& v) {
    return glmToJson(v);
}

quat quatFromJsonValue(const QJsonValue& q) {
    return glmFromJson<quat>(q);
}

vec3 vec3FromJsonValue(const QJsonValue& v) {
    if (v.isDouble()) {
        return vec3((float)v.toDouble());
    }
    return glmFromJson<vec3>(v);
}

