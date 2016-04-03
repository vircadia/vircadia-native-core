#include "DisplayPlugin.h"

#include <ui/Menu.h>

#include "PluginContainer.h"

bool DisplayPlugin::activate() {
    if (isHmd() && (getHmdScreen() >= 0)) {
        _container->showDisplayPluginsTools();
    }
    return Parent::activate();
}

void DisplayPlugin::deactivate() {
    _container->showDisplayPluginsTools(false);
    if (!_container->currentDisplayActions().isEmpty()) {
        auto menu = _container->getPrimaryMenu();
        foreach(auto itemInfo, _container->currentDisplayActions()) {
            menu->removeMenuItem(itemInfo.first, itemInfo.second);
        }
        _container->currentDisplayActions().clear();
    }
    Parent::deactivate();
}


