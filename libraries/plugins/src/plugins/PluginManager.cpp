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
#include <shared/QtHelpers.h>

//#define HIFI_PLUGINMANAGER_DEBUG
#if defined(HIFI_PLUGINMANAGER_DEBUG)
#include <QJsonDocument>
#endif

#include <DependencyManager.h>
#include <UserActivityLogger.h>
#include <QThreadPool>

#include "RuntimePlugin.h"
#include "CodecPlugin.h"
#include "DisplayPlugin.h"
#include "InputPlugin.h"
#include "PluginLogging.h"


void PluginManager::setDisplayPluginProvider(const DisplayPluginProvider& provider) {
    _displayPluginProvider = provider;
}

void PluginManager::setInputPluginProvider(const InputPluginProvider& provider) {
    _inputPluginProvider = provider;
}

void PluginManager::setCodecPluginProvider(const CodecPluginProvider& provider) {
    _codecPluginProvider = provider;
}

void PluginManager::setInputPluginSettingsPersister(const InputPluginSettingsPersister& persister) {
    _inputSettingsPersister = persister;
}

PluginManagerPointer PluginManager::getInstance() {
    return DependencyManager::get<PluginManager>();
}

QString getPluginNameFromMetaData(const QJsonObject& object) {
    static const char* METADATA_KEY = "MetaData";
    static const char* NAME_KEY = "name";
    return object[METADATA_KEY][NAME_KEY].toString("");
}

QString getPluginIIDFromMetaData(const QJsonObject& object) {
    static const char* IID_KEY = "IID";
    return object[IID_KEY].toString("");
}

int getPluginInterfaceVersionFromMetaData(const QJsonObject& object) {
    static const QString METADATA_KEY = "MetaData";
    static const QString NAME_KEY = "version";
    return object[METADATA_KEY][NAME_KEY].toInt(0);
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

int PluginManager::instantiate() {
    auto loaders = getLoadedPlugins();
    return std::count_if(loaders.begin(), loaders.end(), [](const auto& loader) { return (bool)loader->instance(); });
}

 auto PluginManager::getLoadedPlugins() const -> const LoaderList& {
    static std::once_flag once;
    static LoaderList loadedPlugins;
    std::call_once(once, [&] {
#if defined(Q_OS_ANDROID)
        QString pluginPath = QCoreApplication::applicationDirPath() + "/";
#elif defined(Q_OS_MAC)
        QString pluginPath = QCoreApplication::applicationDirPath() + "/../PlugIns/";
#else
        QString pluginPath = QCoreApplication::applicationDirPath() + "/plugins/";
#endif
        QDir pluginDir(pluginPath);
        pluginDir.setSorting(QDir::Name);
        pluginDir.setFilter(QDir::Files);
        if (pluginDir.exists()) {
            qInfo() << "Loading runtime plugins from " << pluginPath;
#if defined(Q_OS_ANDROID)
            // Can be a better filter and those libs may have a better name to destinguish them from qt plugins
            pluginDir.setNameFilters(QStringList() << "libplugins_lib*.so");
#endif
            auto candidates = pluginDir.entryList();

            if (_enableScriptingPlugins.get()) {
                QDir scriptingPluginDir{ pluginDir };
                scriptingPluginDir.cd("scripting");
                qCDebug(plugins) << "Loading scripting plugins from " << scriptingPluginDir.path();
                for (auto plugin : scriptingPluginDir.entryList()) {
                    candidates << "scripting/" + plugin;
                }
            }

            for (auto plugin : candidates) {
                qCDebug(plugins) << "Attempting plugin" << qPrintable(plugin);
                auto loader = QSharedPointer<QPluginLoader>::create(pluginPath + plugin);
                const QJsonObject pluginMetaData = loader->metaData();
#if defined(HIFI_PLUGINMANAGER_DEBUG)
                QJsonDocument metaDataDoc(pluginMetaData);
                qCInfo(plugins) << "Metadata for " << qPrintable(plugin) << ": " << QString(metaDataDoc.toJson());
#endif
                if (isDisabled(pluginMetaData)) {
                    qCWarning(plugins) << "Plugin" << qPrintable(plugin) << "is disabled";
                    // Skip this one, it's disabled
                    continue;
                }

                if (!_pluginFilter(pluginMetaData)) {
                    qCDebug(plugins) << "Plugin" << qPrintable(plugin) << "doesn't pass provided filter";
                    continue;
                }

                if (getPluginInterfaceVersionFromMetaData(pluginMetaData) != HIFI_PLUGIN_INTERFACE_VERSION) {
                    qCWarning(plugins) << "Plugin" << qPrintable(plugin) << "interface version doesn't match, not loading:"
                                       << getPluginInterfaceVersionFromMetaData(pluginMetaData)
                                       << "doesn't match" << HIFI_PLUGIN_INTERFACE_VERSION;
                    continue;
                }

                if (loader->load()) {
                    qCDebug(plugins) << "Plugin" << qPrintable(plugin) << "loaded successfully";
                    loadedPlugins.push_back(loader);
                } else {
                    qCDebug(plugins) << "Plugin" << qPrintable(plugin) << "failed to load:";
                    qCDebug(plugins) << " " << qPrintable(loader->errorString());
                }
            }
        } else {
            qWarning() << "pluginPath does not exit..." << pluginDir;
        }
    });
    return loadedPlugins;
}

const CodecPluginList& PluginManager::getCodecPlugins() {
    static CodecPluginList codecPlugins;
    static std::once_flag once;
    std::call_once(once, [&] {
        codecPlugins = _codecPluginProvider();

        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            CodecProvider* codecProvider = qobject_cast<CodecProvider*>(loader->instance());
            if (codecProvider) {
                for (const auto& codecPlugin : codecProvider->getCodecPlugins()) {
                    if (codecPlugin->isSupported()) {
                        codecPlugins.push_back(codecPlugin);
                    }
                }
            }
        }

        for (auto plugin : codecPlugins) {
            plugin->setContainer(_container);
            plugin->init();

            qCDebug(plugins) << "init codec:" << plugin->getName();
        }
    });
    return codecPlugins;
}

const SteamClientPluginPointer PluginManager::getSteamClientPlugin() {
    static SteamClientPluginPointer steamClientPlugin;
    static std::once_flag once;
    std::call_once(once, [&] {
        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            SteamClientProvider* steamClientProvider = qobject_cast<SteamClientProvider*>(loader->instance());
            if (steamClientProvider) {
                steamClientPlugin = steamClientProvider->getSteamClientPlugin();
                break;
            }
        }
    });
    return steamClientPlugin;
}

const OculusPlatformPluginPointer PluginManager::getOculusPlatformPlugin() {
    static OculusPlatformPluginPointer oculusPlatformPlugin;
    static std::once_flag once;
    std::call_once(once, [&] {
        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            OculusPlatformProvider* oculusPlatformProvider = qobject_cast<OculusPlatformProvider*>(loader->instance());
            if (oculusPlatformProvider) {
                oculusPlatformPlugin = oculusPlatformProvider->getOculusPlatformPlugin();
                break;
            }
        }
    });
    return oculusPlatformPlugin;
}

DisplayPluginList PluginManager::getAllDisplayPlugins() {
    return _displayPlugins;
}

 const DisplayPluginList& PluginManager::getDisplayPlugins()  {
    static std::once_flag once;
    static auto deviceAddedCallback = [](QString deviceName) {
        qCDebug(plugins) << "Added device: " << deviceName;
        UserActivityLogger::getInstance().connectedDevice("display", deviceName);
    };
    static auto subdeviceAddedCallback = [](QString pluginName, QString deviceName) {
        qCDebug(plugins) << "Added subdevice: " << deviceName;
        UserActivityLogger::getInstance().connectedDevice("display", pluginName + " | " + deviceName);
    };

    std::call_once(once, [&] {
        // Grab the built in plugins
        _displayPlugins = _displayPluginProvider();


        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            DisplayProvider* displayProvider = qobject_cast<DisplayProvider*>(loader->instance());
            if (displayProvider) {
                for (const auto& displayPlugin : displayProvider->getDisplayPlugins()) {
                    _displayPlugins.push_back(displayPlugin);
                }
            }
        }
        for (auto plugin : _displayPlugins) {
            connect(plugin.get(), &Plugin::deviceConnected, this, deviceAddedCallback, Qt::QueuedConnection);
            connect(plugin.get(), &Plugin::subdeviceConnected, this, subdeviceAddedCallback, Qt::QueuedConnection);
            plugin->setContainer(_container);
            plugin->init();
        }

    });
    return _displayPlugins;
}

void PluginManager::disableDisplayPlugin(const QString& name) {
    auto it = std::remove_if(_displayPlugins.begin(), _displayPlugins.end(), [&](const DisplayPluginPointer& plugin){
        return plugin->getName() == name;
    });
    _displayPlugins.erase(it, _displayPlugins.end());
}


const InputPluginList& PluginManager::getInputPlugins() {
    static std::once_flag once;
    static auto deviceAddedCallback = [&](QString deviceName) {
        qCDebug(plugins) << "Added device: " << deviceName;
        QStringList runningDevices = getRunningInputDeviceNames();
        bool isDeviceRunning = runningDevices.indexOf(deviceName) >= 0;
        emit inputDeviceRunningChanged(deviceName, isDeviceRunning, runningDevices);
        UserActivityLogger::getInstance().connectedDevice("input", deviceName);
    };
    static auto subdeviceAddedCallback = [](QString pluginName, QString deviceName) {
        qCDebug(plugins) << "Added subdevice: " << deviceName;
        UserActivityLogger::getInstance().connectedDevice("input", pluginName + " | " + deviceName);
    };

    std::call_once(once, [&] {
        _inputPlugins = _inputPluginProvider();

        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            InputProvider* inputProvider = qobject_cast<InputProvider*>(loader->instance());
            if (inputProvider) {
                for (const auto& inputPlugin : inputProvider->getInputPlugins()) {
                    if (inputPlugin->isSupported()) {
                        _inputPlugins.push_back(inputPlugin);
                    }
                }
            }
        }

        for (auto plugin : _inputPlugins) {
            connect(plugin.get(), &Plugin::deviceConnected, this, deviceAddedCallback, Qt::QueuedConnection);
            connect(plugin.get(), &Plugin::subdeviceConnected, this, subdeviceAddedCallback, Qt::QueuedConnection);
            connect(plugin.get(), &Plugin::deviceStatusChanged, this, [&](const QString& deviceName, bool isRunning) {
                emit inputDeviceRunningChanged(deviceName, isRunning, getRunningInputDeviceNames());
            }, Qt::QueuedConnection);
            plugin->setContainer(_container);
            plugin->init();
        }
    });
    return _inputPlugins;
}

QStringList PluginManager::getRunningInputDeviceNames() const {
    QStringList runningDevices;
    for (auto plugin: _inputPlugins) {
        if (plugin->isRunning()) {
            runningDevices << plugin->getName();
        }
    }
    return runningDevices;
}

void PluginManager::setPreferredDisplayPlugins(const QStringList& displays) {
    preferredDisplayPlugins = displays;
}

DisplayPluginList PluginManager::getPreferredDisplayPlugins() {
    static DisplayPluginList displayPlugins;

    static std::once_flag once;
    std::call_once(once, [&] {
        // Grab the built in plugins
        const auto& plugins = getDisplayPlugins();

        for (const auto& pluginName : preferredDisplayPlugins) {
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
    _inputSettingsPersister(getInputPlugins());
}

void PluginManager::shutdown() {
    for (const auto& inputPlugin : getInputPlugins()) {
        if (inputPlugin->isActive()) {
            inputPlugin->deactivate();
        }
    }

    for (const auto& displayPlugins : getDisplayPlugins()) {
        if (displayPlugins->isActive()) {
            displayPlugins->deactivate();
        }
    }

    auto loadedPlugins = getLoadedPlugins();
    // Now grab the dynamic plugins
    for (auto loader : getLoadedPlugins()) {
        InputProvider* inputProvider = qobject_cast<InputProvider*>(loader->instance());
        if (inputProvider) {
            inputProvider->destroyInputPlugins();
        }
        DisplayProvider* displayProvider = qobject_cast<DisplayProvider*>(loader->instance());
        if (displayProvider) {
            displayProvider->destroyDisplayPlugins();
        }
    }
}
