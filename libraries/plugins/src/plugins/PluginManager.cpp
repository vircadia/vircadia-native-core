#include "PluginManager.h"
#include <mutex>


PluginManager* PluginManager::getInstance() {
    static PluginManager _manager;
    return &_manager;
}

// TODO migrate to a DLL model where plugins are discovered and loaded at runtime by the PluginManager class
extern DisplayPluginList getDisplayPlugins();
extern InputPluginList getInputPlugins();

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

