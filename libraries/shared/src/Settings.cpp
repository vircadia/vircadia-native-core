//
//  Settings.cpp
//
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "Settings.h"

QVariant SettingsBridge::getFromSettings(const QString& key, const QVariant& defaultValue) {
    return QSettings().value(key, defaultValue);
}

void SettingsBridge::setInSettings(const QString& key, const QVariant& value) {
    QSettings().setValue(key, value);
}