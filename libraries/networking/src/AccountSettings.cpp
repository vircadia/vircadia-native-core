//
//  AccountSettings.cpp
//  libraries/networking/src
//
//  Created by Clement Brisset on 9/12/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AccountSettings.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "NetworkLogging.h"

// Examples:
//static QString SOME_STRING_KEY { "some_string" };
//static QString SOME_INT_KEY { "some_int" };
//static QString SOME_BOOL_KEY { "some_bool" };

// Examples:
//QString AccountSettings::DEFAULT_SOME_STRING { "" };
//int AccountSettings::DEFAULT_SOME_INT { 17 };
//bool AccountSettings::DEFAULT_SOME_BOOL { true };

QJsonObject AccountSettings::pack() {
    QJsonObject data;

    QReadLocker lock(&_settingsLock);
//    Examples:
//    data.insert(SOME_STRING_KEY, _someString);
//    data.insert(SOME_INT_KEY, _someInt);
//    data.insert(SOME_BOOL_KEY, _someBool);

    return data;
}

void AccountSettings::unpack(QJsonObject data) {
    QWriteLocker lock(&_settingsLock);

//    Examples:
//    auto it = data.find(SOME_STRING_KEY);
//    _hasSomeString = it != data.end() && it->isString();
//    _someString = _hasSomeString ? it->toString() : DEFAULT_SOME_STRING;
//
//    it = data.find(SOME_INT_KEY);
//    _hasSomeInt = it != data.end() && it->isDouble();
//    _someInt = _hasSomeInt ? it->toInt() : DEFAULT_SOME_INT;
//
//    it = data.find(SOME_BOOL_KEY);
//    _hasSomeBool = it != data.end() && it->isBool();
//    _someBool = _hasSomeBool ? it->toBool() : DEFAULT_SOME_BOOL;

    _somethingChanged = false;
}

// Examples:
//void AccountSettings::setSomeString(QString someString) {
//    QWriteLocker lock(&_settingsLock);
//    if (someString != _someString) {
//        _somethingChanged = true;
//    }
//    _someString = someString;
//}
//
//void AccountSettings::setSomeInt(int someInt) {
//    QWriteLocker lock(&_settingsLock);
//    if (someInt != _someInt) {
//        _somethingChanged = true;
//    }
//    _someInt = someInt;
//}
//
//void AccountSettings::setSomeBool(bool someBool) {
//    QWriteLocker lock(&_settingsLock);
//    if (someBool != _someBool) {
//        _somethingChanged = true;
//    }
//    _someBool = someBool;
//}
