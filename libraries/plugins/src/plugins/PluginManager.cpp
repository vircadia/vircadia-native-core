//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PluginManager.h"

#include <mutex>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QPluginLoader>

#include <DependencyManager.h>
#include <UserActivityLogger.h>

#include "RuntimePlugin.h"
#include "CodecPlugin.h"
#include "DisplayPlugin.h"
#include "InputPlugin.h"


PluginManager* PluginManager::getInstance() {
    static PluginManager _manager;
    return &_manager;
}

QString getPluginNameFromMetaData(QJsonObject object) {
    static const char* METADATA_KEY = "MetaData";
    static const char* NAME_KEY = "name";

    if (!object.contains(METADATA_KEY) || !object[METADATA_KEY].isObject()) {
        return QString();
    }

    auto metaDataObject = object[METADATA_KEY].toObject();

    if (!metaDataObject.contains(NAME_KEY) || !metaDataObject[NAME_KEY].isString()) {
        return QString();
    }

    return metaDataObject[NAME_KEY].toString();
}

QString getPluginIIDFromMetaData(QJsonObject object) {
    static const char* IID_KEY = "IID";

    if (!object.contains(IID_KEY) || !object[IID_KEY].isString()) {
        return QString();
    }

    return object[IID_KEY].toString();
}

QStringList preferredDisplayPlugins;
QStringList disabledDisplays;
QStringList disabledInputs;

bool isDisabled(QJsonObject metaData) {
    auto name = getPluginNameFromMetaData(metaData);
    auto iid = getPluginIIDFromMetaData(metaData);

    if (iid == DisplayProvider_iid) {
        return disabledDisplays.contains(name);
    } else if (iid == InputProvider_iid) {
        return disabledInputs.contains(name);
    }

    return false;
}

using Loader = QSharedPointer<QPluginLoader>;
using LoaderList = QList<Loader>;

const LoaderList& getLoadedPlugins() {
    static std::once_flag once;
    static LoaderList loadedPlugins;
    std::call_once(once, [&] {
#ifdef Q_OS_MAC
        QString pluginPath = QCoreApplication::applicationDirPath() + "/../PlugIns/";
#else
        QString pluginPath = QCoreApplication::applicationDirPath() + "/plugins/";
#endif
        QDir pluginDir(pluginPath);
        pluginDir.setFilter(QDir::Files);
        if (pluginDir.exists()) {
            qDebug() << "Loading runtime plugins from " << pluginPath;
            auto candidates = pluginDir.entryList();
            for (auto plugin : candidates) {
                qDebug() << "Attempting plugin" << qPrintable(plugin);
                QSharedPointer<QPluginLoader> loader(new QPluginLoader(pluginPath + plugin));

                if (isDisabled(loader->metaData())) {
                    qWarning() << "Plugin" << qPrintable(plugin) << "is disabled";
                    // Skip this one, it's disabled
                    continue;
                }

                if (loader->load()) {
                    qDebug() << "Plugin" << qPrintable(plugin) << "loaded successfully";
                    loadedPlugins.push_back(loader);
                } else {
                    qDebug() << "Plugin" << qPrintable(plugin) << "failed to load:";
                    qDebug() << " " << qPrintable(loader->errorString());
                }
            }
        }
    });
    return loadedPlugins;
}

PluginManager::PluginManager() {
}

#ifndef Q_OS_ANDROID

// TODO migrate to a DLL model where plugins are discovered and loaded at runtime by the PluginManager class
extern DisplayPluginList getDisplayPlugins();
extern InputPluginList getInputPlugins();
extern CodecPluginList getCodecPlugins();
extern void saveInputPluginSettings(const InputPluginList& plugins);
static DisplayPluginList displayPlugins;

const DisplayPluginList& PluginManager::getDisplayPlugins() {
    static std::once_flag once;
    static auto deviceAddedCallback = [](QString deviceName) {
        qDebug() << "Added device: " << deviceName;
        UserActivityLogger::getInstance().connectedDevice("display", deviceName);
    };
    static auto subdeviceAddedCallback = [](QString pluginName, QString deviceName) {
        qDebug() << "Added subdevice: " << deviceName;
        UserActivityLogger::getInstance().connectedDevice("display", pluginName + " | " + deviceName);
    };

    std::call_once(once, [&] {
        // Grab the built in plugins
        displayPlugins = ::getDisplayPlugins();


        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            DisplayProvider* displayProvider = qobject_cast<DisplayProvider*>(loader->instance());
            if (displayProvider) {
                for (auto displayPlugin : displayProvider->getDisplayPlugins()) {
                    displayPlugins.push_back(displayPlugin);
                }
            }
        }
        for (auto plugin : displayPlugins) {
            connect(plugin.get(), &Plugin::deviceConnected, this, deviceAddedCallback, Qt::QueuedConnection);
            connect(plugin.get(), &Plugin::subdeviceConnected, this, subdeviceAddedCallback, Qt::QueuedConnection);
            plugin->setContainer(_container);
            plugin->init();
        }

    });
    return displayPlugins;
}

void PluginManager::disableDisplayPlugin(const QString& name) {
    for (size_t i = 0; i < displayPlugins.size(); ++i) {
        if (displayPlugins[i]->getName() == name) {
            displayPlugins.erase(displayPlugins.begin() + i);
            break;
        }
    }
}


const InputPluginList& PluginManager::getInputPlugins() {
    static InputPluginList inputPlugins;
    static std::once_flag once;
    static auto deviceAddedCallback = [](QString deviceName) {
        qDebug() << "Added device: " << deviceName;
        UserActivityLogger::getInstance().connectedDevice("input", deviceName);
    };
    static auto subdeviceAddedCallback = [](QString pluginName, QString deviceName) {
        qDebug() << "Added subdevice: " << deviceName;
        UserActivityLogger::getInstance().connectedDevice("input", pluginName + " | " + deviceName);
    };

    std::call_once(once, [&] {
        inputPlugins = ::getInputPlugins();

        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            InputProvider* inputProvider = qobject_cast<InputProvider*>(loader->instance());
            if (inputProvider) {
                for (auto inputPlugin : inputProvider->getInputPlugins()) {
                    if (inputPlugin->isSupported()) {
                        inputPlugins.push_back(inputPlugin);
                    }
                }
            }
        }

        for (auto plugin : inputPlugins) {
            connect(plugin.get(), &Plugin::deviceConnected, this, deviceAddedCallback, Qt::QueuedConnection);
            connect(plugin.get(), &Plugin::subdeviceConnected, this, subdeviceAddedCallback, Qt::QueuedConnection);
            plugin->setContainer(_container);
            plugin->init();
        }
    });
    return inputPlugins;
}

const CodecPluginList& PluginManager::getCodecPlugins() {
    static CodecPluginList codecPlugins;
    static std::once_flag once;
    std::call_once(once, [&] {
        //codecPlugins = ::getCodecPlugins();

        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            CodecProvider* codecProvider = qobject_cast<CodecProvider*>(loader->instance());
            if (codecProvider) {
                for (auto codecPlugin : codecProvider->getCodecPlugins()) {
                    if (codecPlugin->isSupported()) {
                        codecPlugins.push_back(codecPlugin);
                    }
                }
            }
        }

        for (auto plugin : codecPlugins) {
            plugin->setContainer(_container);
            plugin->init();

            qDebug() << "init codec:" << plugin->getName();
        }
    });
    return codecPlugins;
}


void PluginManager::setPreferredDisplayPlugins(const QStringList& displays) {
    preferredDisplayPlugins = displays;
}

DisplayPluginList PluginManager::getPreferredDisplayPlugins() {
    static DisplayPluginList displayPlugins;

    static std::once_flag once;
    std::call_once(once, [&] {
        // Grab the built in plugins
        auto plugins = getDisplayPlugins();

        for (auto pluginName : preferredDisplayPlugins) {
            auto it = std::find_if(plugins.begin(), plugins.end(), [&](DisplayPluginPointer plugin) {
                return plugin->getName() == pluginName;
            });
            if (it != plugins.end()) {
                displayPlugins.push_back(*it);
            }
        }
    });

    return displayPlugins;
}


void PluginManager::disableDisplays(const QStringList& displays) {
    disabledDisplays << displays;
}

void PluginManager::disableInputs(const QStringList& inputs) {
    disabledInputs << inputs;
}

void PluginManager::saveSettings() {
    saveInputPluginSettings(getInputPlugins());
}

#endif
