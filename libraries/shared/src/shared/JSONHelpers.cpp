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

#include <QtCore/qmetaobject.h>

#include "../RegisteredMetaTypes.h"

template <typename T> 
QJsonValue glmToJson(const T& t) {
    QJsonArray result;
    for (auto i = 0; i < t.length(); ++i) {
        result.push_back(t[i]);
    }
    return result;
}

template <typename T> 
T glmFromJson(const QJsonValue& json) {
    T result;
    if (json.isArray()) {
        QJsonArray array = json.toArray();
        auto length = std::min(array.size(), result.length());
        for (auto i = 0; i < length; ++i) {
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

QJsonValue toJsonValue(const vec4& v) {
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

vec4 vec4FromJsonValue(const QJsonValue& v) {
    if (v.isDouble()) {
        return vec4((float)v.toDouble());
    }
    return glmFromJson<vec4>(v);
}

QJsonValue toJsonValue(const QObject& o) {
    QJsonObject json{};

    // Add all properties, see http://doc.qt.io/qt-5/qmetaobject.html#propertyCount
    const auto& meta = o.metaObject();
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        QString name = QString::fromLatin1(meta->property(i).name());
        auto type = meta->property(i).userType();
        QVariant variant{ meta->property(i).read(&o) };
        QJsonValue value;

        // User-registered types need explicit conversion
        if (type == qMetaTypeId<quat>()) {
            value = toJsonValue(variant.value<quat>());
        } else if (type == qMetaTypeId<vec3>()) {
            value = toJsonValue(variant.value<vec3>());
        } else if (type == qMetaTypeId<vec4>()) {
            value = toJsonValue(variant.value<vec4>());
        } else {
            // Qt types are converted automatically
            value = QJsonValue::fromVariant(variant);
        }

        json.insert(name, value);
    }

    // Add all children (recursively)
    const auto children = o.children();
    for (const auto& child : children) {
        QJsonObject childJson = toJsonValue(*child).toObject();
        if (!childJson.empty()) {
            json.insert(child->objectName(), childJson);
        }
    }

    return json;
}

void qObjectFromJsonValue(const QJsonValue& j, QObject& o) {
    const QJsonObject object = j.toObject();
    for (auto it = object.begin(); it != object.end(); it++) {
        std::string key = it.key().toStdString();
        if (it.value().isObject()) {
            QObject* child = o.findChild<QObject*>(key.c_str(), Qt::FindDirectChildrenOnly);
            if (child) {
                qObjectFromJsonValue(it.value(), *child);
            }
        } else {
            o.setProperty(key.c_str(), it.value());
        }
    }
}
