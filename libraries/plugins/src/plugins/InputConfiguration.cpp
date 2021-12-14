//
//  Created by Dante Ruiz on 6/1/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "InputConfiguration.h"

#include <QThread>

#include <shared/QtHelpers.h>

#include "DisplayPlugin.h"
#include "InputPlugin.h"
#include "PluginManager.h"

InputConfiguration::InputConfiguration() {
}

QStringList InputConfiguration::inputPlugins() {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        BLOCKING_INVOKE_METHOD(this, "inputPlugins",
                                  Q_RETURN_ARG(QStringList, result));
        return result;
    }

    QStringList inputPlugins;
    for (const auto& plugin : PluginManager::getInstance()->getInputPlugins()) {
        QString pluginName = plugin->getName();
        if (pluginName == QString("OpenVR")) {
            QString headsetName = plugin->getDeviceName();
            inputPlugins << headsetName;
        } else {
            inputPlugins << pluginName;
        }
    }
    return inputPlugins;
}


QStringList InputConfiguration::activeInputPlugins() {
    if (QThread::currentThread() != thread()) {
        QStringList result;
        BLOCKING_INVOKE_METHOD(this, "activeInputPlugins",
                                   Q_RETURN_ARG(QStringList, result));
        return result;
    }

    QStringList activePlugins;
    for (const auto& plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->configurable()) {
            QString pluginName = plugin->getName();
            if (pluginName == QString("OpenVR")) {
                QString headsetName = plugin->getDeviceName();
                activePlugins << headsetName;
            } else {
                activePlugins << pluginName;
            }
        }
    }
    return activePlugins;
}

QString InputConfiguration::configurationLayout(QString pluginName) {
    if (QThread::currentThread() != thread()) {
        QString result;
        BLOCKING_INVOKE_METHOD(this, "configurationLayout",
                                  Q_RETURN_ARG(QString, result),
                                  Q_ARG(QString, pluginName));
        return result;
    }

    QString sourcePath;
    for (const auto& plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName || plugin->getDeviceName() == pluginName) {
            return plugin->configurationLayout();
        }
    }
    return sourcePath;
}

void InputConfiguration::setConfigurationSettings(QJsonObject configurationSettings, QString pluginName) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "setConfigurationSettings",
                                  Q_ARG(QJsonObject, configurationSettings),
                                  Q_ARG(QString, pluginName));
        return;
    }

    for (const auto& plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName) {
            plugin->setConfigurationSettings(configurationSettings);
        }
    }
}

QJsonObject InputConfiguration::configurationSettings(QString pluginName) {
    if (QThread::currentThread() != thread()) {
        QJsonObject result;
        BLOCKING_INVOKE_METHOD(this, "configurationSettings",
                                  Q_RETURN_ARG(QJsonObject, result),
                                  Q_ARG(QString, pluginName));
        return result;
    }

    for (const auto& plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName) {
            return plugin->configurationSettings();
        }
    }
    return QJsonObject();
}

void InputConfiguration::calibratePlugin(QString pluginName) {
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(this, "calibratePlugin");
        return;
    }

    for (const auto& plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName) {
            plugin->calibrate();
        }
    }
}


bool InputConfiguration::uncalibratePlugin(QString pluginName) {
    if (QThread::currentThread() != thread()) {
        bool result;
        BLOCKING_INVOKE_METHOD(this, "uncalibratePlugin",
                                  Q_ARG(bool, result));
        return result;
    }

    for (const auto& plugin : PluginManager::getInstance()->getInputPlugins()) {
        if (plugin->getName() == pluginName) {
            return plugin->uncalibrate();
        }
    }
    
    return false;
}
