//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QObject>
#include <QtCore/QSharedPointer>

#include <DependencyManager.h>
#include <SettingHandle.h>

#include "Forward.h"

class QPluginLoader;
using PluginManagerPointer = QSharedPointer<PluginManager>;

class PluginManager : public QObject, public Dependency {
    SINGLETON_DEPENDENCY
    Q_OBJECT

public:
    static PluginManagerPointer getInstance();

    const DisplayPluginList& getDisplayPlugins();
    const InputPluginList& getInputPlugins();
    const CodecPluginList& getCodecPlugins();
    const SteamClientPluginPointer getSteamClientPlugin();
    const OculusPlatformPluginPointer getOculusPlatformPlugin();

    DisplayPluginList getPreferredDisplayPlugins();
    void setPreferredDisplayPlugins(const QStringList& displays);

    void disableDisplayPlugin(const QString& name);
    void disableDisplays(const QStringList& displays);
    void disableInputs(const QStringList& inputs);
    void saveSettings();
    void setContainer(PluginContainer* container) { _container = container; }

    int instantiate();
    void shutdown();

    // Application that have statically linked plugins can expose them to the plugin manager with these function
    void setDisplayPluginProvider(const DisplayPluginProvider& provider);
    void setInputPluginProvider(const InputPluginProvider& provider);
    void setCodecPluginProvider(const CodecPluginProvider& provider);
    void setInputPluginSettingsPersister(const InputPluginSettingsPersister& persister);
    QStringList getRunningInputDeviceNames() const;

    using PluginFilter = std::function<bool(const QJsonObject&)>;
    void setPluginFilter(PluginFilter pluginFilter) { _pluginFilter = pluginFilter; }
    Q_INVOKABLE DisplayPluginList getAllDisplayPlugins();

signals:
    void inputDeviceRunningChanged(const QString& pluginName, bool isRunning, const QStringList& runningDevices);
    
private:
    PluginManager() = default;

    DisplayPluginProvider _displayPluginProvider { []()->DisplayPluginList { return {}; } };
    InputPluginProvider _inputPluginProvider { []()->InputPluginList { return {}; } };
    CodecPluginProvider _codecPluginProvider { []()->CodecPluginList { return {}; } };
    InputPluginSettingsPersister _inputSettingsPersister { [](const InputPluginList& list) {} };
    PluginContainer* _container { nullptr };
    DisplayPluginList _displayPlugins;
    InputPluginList _inputPlugins;
    PluginFilter _pluginFilter { [](const QJsonObject&) { return true; } };

    using Loader = QSharedPointer<QPluginLoader>;
    using LoaderList = QList<Loader>;

    const LoaderList& getLoadedPlugins() const;
    Setting::Handle<bool> _enableScriptingPlugins {
        "private/enableScriptingPlugins", (bool)qgetenv("enableScriptingPlugins").toInt()
    };
};

// TODO: we should define this value in CMake, and then use CMake
// templating to generate the individual plugin.json files, so that we
// don't have to update every plugin.json file whenever we update this
// value.  The value should match "version" in
//   plugins/*/src/plugin.json
//   plugins/oculus/src/oculus.json
//   etc
static const int HIFI_PLUGIN_INTERFACE_VERSION = 1;
