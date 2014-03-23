//
//  SettingsScriptingInterface.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/25/14
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "Application.h"
#include "SettingsScriptingInterface.h"


SettingsScriptingInterface* SettingsScriptingInterface::getInstance() {
    static SettingsScriptingInterface sharedInstance;
    return &sharedInstance;
}

QVariant SettingsScriptingInterface::getValue(const QString& setting) {
    QSettings* settings = Application::getInstance()->lockSettings();
    QVariant value = settings->value(setting);
    Application::getInstance()->unlockSettings();
    return value;
}

QVariant SettingsScriptingInterface::getValue(const QString& setting, const QVariant& defaultValue) {
    QSettings* settings = Application::getInstance()->lockSettings();
    QVariant value = settings->value(setting, defaultValue);
    Application::getInstance()->unlockSettings();
    return value;
}

void SettingsScriptingInterface::setValue(const QString& setting, const QVariant& value) {
    QSettings* settings = Application::getInstance()->lockSettings();
    settings->setValue(setting, value);
    Application::getInstance()->unlockSettings();
}
