//
//  JSONBreakableMarshal.h 
//  libraries/networking/src
//
//  Created by Stephen Birarda on 04/28/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JSONBreakableMarshal_h
#define hifi_JSONBreakableMarshal_h

#pragma once

#include <QtCore/QJsonValue>
#include <QtCore/QString>
#include <QtCore/QStringList>

class JSONBreakableMarshal {
public:
    static QStringList toStringList(const QJsonValue& jsonValue, const QString& keypath);
    static QString toString(const QJsonValue& jsonValue, const QString& keyPath);
    static QJsonValue fromString(const QString& marshalValue);
    static QJsonObject fromStringList(const QStringList& stringList);
    static QJsonObject fromStringBuffer(const QByteArray& buffer);
};

#endif // hifi_JSONBreakableMarshal_h
