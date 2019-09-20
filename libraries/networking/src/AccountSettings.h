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
    enum State {
        LoggedOut,
        Loading,
        Loaded,
        NotPresent
    };

    void loggedOut();
    void startedLoading();
    quint64 lastChangeTimestamp() const { return _lastChangeTimestamp; }

    QJsonObject pack();
    void unpack(QJsonObject data);

    State homeLocationState() const { QReadLocker lock(&_settingsLock); return _homeLocationState; }
    QString getHomeLocation() const { QReadLocker lock(&_settingsLock); return _homeLocation; }
    void setHomeLocation(QString homeLocation);

private:
    mutable QReadWriteLock _settingsLock;
    quint64 _lastChangeTimestamp { 0 };

    State _homeLocationState { LoggedOut };
    QString _homeLocation;
};

#endif /* hifi_AccountSettings_h */
