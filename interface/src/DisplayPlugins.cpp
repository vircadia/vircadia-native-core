//
//  Created by Bradley Austin Davis on 2015/05/30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DisplayPlugins.h"

#include "Application.h"

#include <display-plugins/DisplayPlugin.h>
#include <display-plugins/NullDisplayPlugin.h>
#include <display-plugins/stereo/SideBySideStereoDisplayPlugin.h>
#include <display-plugins/stereo/InterleavedStereoDisplayPlugin.h>
#include <display-plugins/Basic2DWindowOpenGLDisplayPlugin.h>

#include <display-plugins/openvr/OpenVrDisplayPlugin.h>

extern DisplayPlugin* makeOculusDisplayPlugin();

static void addDisplayPluginToMenu(DisplayPluginPointer displayPlugin, bool active = false) {
    auto menu = Menu::getInstance();
    QString name = displayPlugin->getName();
    Q_ASSERT(!menu->menuItemExists(MenuOption::OutputMenu, name));

    static QActionGroup* displayPluginGroup = nullptr;
    if (!displayPluginGroup) {
        displayPluginGroup = new QActionGroup(menu);
        displayPluginGroup->setExclusive(true);
    }
    auto parent = menu->getMenu(MenuOption::OutputMenu);
    auto action = menu->addActionToQMenuAndActionHash(parent,
        name, 0, qApp,
        SLOT(updateDisplayMode()));
    action->setCheckable(true);
    action->setChecked(active);
    displayPluginGroup->addAction(action);
    Q_ASSERT(menu->menuItemExists(MenuOption::OutputMenu, name));
}

// FIXME move to a plugin manager class
const DisplayPluginList& getDisplayPlugins() {
    static DisplayPluginList RENDER_PLUGINS;
    static bool init = false;
    if (!init) {
        init = true;

        DisplayPlugin* PLUGIN_POOL[] = {
            new Basic2DWindowOpenGLDisplayPlugin(),
#ifdef DEBUG
            new NullDisplayPlugin(),
#endif
            // FIXME fix stereo display plugins
            //new SideBySideStereoDisplayPlugin(),
            //new InterleavedStereoDisplayPlugin(),

            makeOculusDisplayPlugin(),  

#ifdef Q_OS_WIN
            new OpenVrDisplayPlugin(),
#endif
            nullptr
        };
        for (int i = 0; PLUGIN_POOL[i]; ++i) {
            DisplayPlugin * plugin = PLUGIN_POOL[i];
            if (plugin->isSupported()) {
                plugin->init();
                QObject::connect(plugin, &DisplayPlugin::requestRender, [] {
                    qApp->paintGL();
                });
                QObject::connect(plugin, &DisplayPlugin::recommendedFramebufferSizeChanged, [](const QSize & size) {
                    qApp->resizeGL();
                });
                DisplayPluginPointer pluginPointer(plugin);
                addDisplayPluginToMenu(pluginPointer, plugin == *PLUGIN_POOL);
                RENDER_PLUGINS.push_back(pluginPointer);
            }
        }
    }
    return RENDER_PLUGINS;
}
