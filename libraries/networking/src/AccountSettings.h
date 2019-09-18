//
//  AccountSettings.h
//  libraries/networking/src
//
//  Created by Clement Brisset on 9/12/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AccountSettings_h
#define hifi_AccountSettings_h

#include <QJsonObject>
#include <QReadWriteLock>
#include <QString>

class AccountSettings {
public:
//    Examples:
//    static QString DEFAULT_SOME_STRING;
//    static int DEFAULT_SOME_INT;
//    static bool DEFAULT_SOME_BOOL;

    bool somethingChanged() const { return _somethingChanged; }

    QJsonObject pack();
    void unpack(QJsonObject data);

//    Examples:
//    bool hasSomeString() const { QReadLocker lock(&_settingsLock); return _hasSomeString; }
//    QString getSomeString() const { QReadLocker lock(&_settingsLock); return _someString; }
//    void setSomeString(QString someString);
//
//    bool hasSomeInt() const { QReadLocker lock(&_settingsLock); return _hasSomeInt; }
//    int getSomeInt() const { QReadLocker lock(&_settingsLock); return _someInt; }
//    void setSomeInt(int someInt);
//
//    bool hasSomeBool() const { QReadLocker lock(&_settingsLock); return _hasSomeBool; }
//    bool getSomeBool() const { QReadLocker lock(&_settingsLock); return _someBool; }
//    void setSomeBool(bool someBool);

private:
    mutable QReadWriteLock _settingsLock;
    bool _somethingChanged { false };

//    Examples:
//    bool _hasSomeString { false };
//    bool _hasSomeInt { false };
//    bool _hasSomeBool { false };

//    Examples:
//    QString _someString { DEFAULT_SOME_STRING };
//    int _someInt { DEFAULT_SOME_INT };
//    bool _someBool { DEFAULT_SOME_BOOL };
};

#endif /* hifi_AccountSettings_h */
