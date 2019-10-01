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
#include "SharedUtil.h"

static QString HOME_LOCATION_KEY { "home_location" };

QJsonObject AccountSettings::pack() {
    QJsonObject data;

    QReadLocker lock(&_settingsLock);
    data.insert(HOME_LOCATION_KEY, _homeLocation);

    return data;
}

void AccountSettings::unpack(QJsonObject data) {
    QWriteLocker lock(&_settingsLock);

    _lastChangeTimestamp = usecTimestampNow();

    auto it = data.find(HOME_LOCATION_KEY);
    _homeLocationState = it != data.end() && it->isString() ? Loaded : NotPresent;
    _homeLocation = _homeLocationState == Loaded ? it->toString() : "";
}

void AccountSettings::setHomeLocation(QString homeLocation) {
    QWriteLocker lock(&_settingsLock);
    if (homeLocation != _homeLocation) {
        _lastChangeTimestamp = usecTimestampNow();
    }
    _homeLocation = homeLocation;
}

void AccountSettings::startedLoading() {
    _homeLocationState = Loading;
}

void AccountSettings::loggedOut() {
    _homeLocationState = LoggedOut;
}
