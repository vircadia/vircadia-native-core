//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "InputConfiguration.h"
#include "DisplayPlugin.h"
#include "InputPlugin.h"
#include "PluginManager.h"

InputConfiguration::InputConfiguration() {
}

QStringList InputConfiguration::inputPlugins() {
    QStringList inputPlugins;
    for (auto plugin : PluginManager::getInstance()->getInputPlugins()) {
        inputPlugins << QString(plugin->getName());
    }
    return inputPlugins;
}


QStringList InputConfiguration::activeInputPlugins() {
    QStringList activePlugins;
    for (auto plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->configurable()) {
            activePlugins << QString(plugin->getName());
        }
    }
    return activePlugins;
}

QString InputConfiguration::configurationLayout(QString pluginName) {
    QString sourcePath; 

    for (auto plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName) {
            return plugin->configurationLayout();
        }
    }
    return sourcePath;
}

void InputConfiguration::setConfigurationSettings(QJsonObject configurationSettings, QString pluginName) {
    for (auto plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName) {
            plugin->setConfigurationSettings(configurationSettings);
        }
    }
}

QJsonObject InputConfiguration::configurationSettings(QString pluginName) {
    for (auto plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName) {
            return plugin->configurationSettings();
        }
    }
    return QJsonObject();
}

void InputConfiguration::calibratePlugin(QString pluginName) {
    for (auto plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName) {
            plugin->calibrate();
        }
    }
}

void InputConfiguration::calibrated(const QJsonObject& status) {
    emit calibrationStatus(status);
}
