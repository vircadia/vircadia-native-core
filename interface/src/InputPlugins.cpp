//
//  Created by Bradley Austin Davis on 2015/05/30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InputPlugins.h"

#include "Application.h"

#include <input-plugins/InputPlugin.h>
#include <input-plugins/KeyboardMouseDevice.h>
#include <input-plugins/SDL2Manager.h>
#include <input-plugins/SixenseManager.h>
#include <input-plugins/ViveControllerManager.h>


static void addInputPluginToMenu(InputPluginPointer inputPlugin, bool active = false) {
    auto menu = Menu::getInstance();
    QString name = inputPlugin->getName();
    Q_ASSERT(!menu->menuItemExists(MenuOption::InputMenu, name));

    static QActionGroup* inputPluginGroup = nullptr;
    if (!inputPluginGroup) {
        inputPluginGroup = new QActionGroup(menu);
    }
    auto parent = menu->getMenu(MenuOption::InputMenu);
    auto action = menu->addCheckableActionToQMenuAndActionHash(parent,
        name, 0, true, qApp,
        SLOT(updateInputModes()));
    inputPluginGroup->addAction(action);
    inputPluginGroup->setExclusive(false);
    Q_ASSERT(menu->menuItemExists(MenuOption::InputMenu, name));
}

// FIXME move to a plugin manager class
const InputPluginList& getInputPlugins() {
    static InputPluginList INPUT_PLUGINS;
    static bool init = false;
    if (!init) {
        init = true;

        InputPlugin* PLUGIN_POOL[] = {
            new KeyboardMouseDevice(),
            new SDL2Manager(),
            new SixenseManager(),
            new ViveControllerManager(),
            nullptr
        };
        for (int i = 0; PLUGIN_POOL[i]; ++i) {
            InputPlugin * plugin = PLUGIN_POOL[i];
            if (plugin->isSupported()) {
                plugin->init();
                InputPluginPointer pluginPointer(plugin);
                addInputPluginToMenu(pluginPointer, plugin == *PLUGIN_POOL);
                INPUT_PLUGINS.push_back(pluginPointer);
            }
        }
    }
    return INPUT_PLUGINS;
}
