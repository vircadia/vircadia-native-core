//
//  SettingsScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 2/25/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <Settings.h>

#include "SettingsScriptingInterface.h"


SettingsScriptingInterface* SettingsScriptingInterface::getInstance() {
    static SettingsScriptingInterface sharedInstance;
    return &sharedInstance;
}

QVariant SettingsScriptingInterface::getValue(const QString& setting) {
    QVariant value = Settings().value(setting);
    if (!value.isValid()) {
        value = "";
    }
    return value;
}

QVariant SettingsScriptingInterface::getValue(const QString& setting, const QVariant& defaultValue) {
    QVariant value = Settings().value(setting, defaultValue);
    if (!value.isValid()) {
        value = "";
    }
    return value;
}

void SettingsScriptingInterface::setValue(const QString& setting, const QVariant& value) {
    Settings().setValue(setting, value);
}
