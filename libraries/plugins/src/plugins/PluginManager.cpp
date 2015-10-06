//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "PluginManager.h"
#include <mutex>

#include "Forward.h"

PluginManager* PluginManager::getInstance() {
    static PluginManager _manager;
    return &_manager;
}

// TODO migrate to a DLL model where plugins are discovered and loaded at runtime by the PluginManager class
extern DisplayPluginList getDisplayPlugins();
extern InputPluginList getInputPlugins();
extern void saveInputPluginSettings(const InputPluginList& plugins);

const DisplayPluginList& PluginManager::getDisplayPlugins() {
    static DisplayPluginList displayPlugins;
    static std::once_flag once;
    std::call_once(once, [&] {
        displayPlugins = ::getDisplayPlugins();
    });
    return displayPlugins;
}

const InputPluginList& PluginManager::getInputPlugins() {
    static InputPluginList inputPlugins;
    static std::once_flag once;
    std::call_once(once, [&] {
        inputPlugins = ::getInputPlugins();
    });
    return inputPlugins;
}

void PluginManager::saveSettings() {
    saveInputPluginSettings(getInputPlugins());
}
