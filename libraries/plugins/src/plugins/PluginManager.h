//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QObject>

#include "Forward.h"

class PluginManager : public QObject {
public:
    static PluginManager* getInstance();
    PluginManager();

    const DisplayPluginList& getDisplayPlugins();
    const InputPluginList& getInputPlugins();
    const CodecPluginList& getCodecPlugins();
    const SteamClientPluginPointer getSteamClientPlugin();

    DisplayPluginList getPreferredDisplayPlugins();
    void setPreferredDisplayPlugins(const QStringList& displays);

    void disableDisplayPlugin(const QString& name);
    void disableDisplays(const QStringList& displays);
    void disableInputs(const QStringList& inputs);
    void saveSettings();
    void setContainer(PluginContainer* container) { _container = container; }

    void shutdown();

    // Application that have statically linked plugins can expose them to the plugin manager with these function
    static void setDisplayPluginProvider(const DisplayPluginProvider& provider);
    static void setInputPluginProvider(const InputPluginProvider& provider);
    static void setCodecPluginProvider(const CodecPluginProvider& provider);
    static void setInputPluginSettingsPersister(const InputPluginSettingsPersister& persister);
    
private:
    static DisplayPluginProvider _displayPluginProvider;
    static InputPluginProvider _inputPluginProvider;  
    static CodecPluginProvider _codecPluginProvider;
    static InputPluginSettingsPersister _inputSettingsPersister;

    PluginContainer* _container { nullptr };
};
