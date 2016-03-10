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

#include "RuntimePlugin.h"
#include "DisplayPlugin.h"
#include "InputPlugin.h"
#include "PluginContainer.h"


PluginManager* PluginManager::getInstance() {
    static PluginManager _manager;
    return &_manager;
}

using Loader = QSharedPointer<QPluginLoader>;
using LoaderList = QList<Loader>;

const LoaderList& getLoadedPlugins() {
    static std::once_flag once;
    static LoaderList loadedPlugins;
    std::call_once(once, [&] {
        QString pluginPath = QCoreApplication::applicationDirPath() + "/plugins/";
        QDir pluginDir(pluginPath);
        pluginDir.setFilter(QDir::Files);
        if (pluginDir.exists()) {
            qDebug() << "Loading runtime plugins from " << pluginPath;
            auto candidates = pluginDir.entryList();
            for (auto plugin : candidates) {
                qDebug() << "Attempting plugins " << plugin;
                QSharedPointer<QPluginLoader> loader(new QPluginLoader(pluginPath + plugin));
                if (loader->load()) {
                    qDebug() << "Plugins " << plugin << " success";
                    loadedPlugins.push_back(loader);
                }
            }
        }
    });
    return loadedPlugins;
}

PluginManager::PluginManager() {
}

// TODO migrate to a DLL model where plugins are discovered and loaded at runtime by the PluginManager class
extern DisplayPluginList getDisplayPlugins();
extern InputPluginList getInputPlugins();
extern void saveInputPluginSettings(const InputPluginList& plugins);

const DisplayPluginList& PluginManager::getDisplayPlugins() {
    static DisplayPluginList displayPlugins;
    static std::once_flag once;

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
        auto& container = PluginContainer::getInstance();
        for (auto plugin : displayPlugins) {
            plugin->setContainer(&container);
            plugin->init();
        }

    });
    return displayPlugins;
}

const InputPluginList& PluginManager::getInputPlugins() {
    static InputPluginList inputPlugins;
    static std::once_flag once;
    std::call_once(once, [&] {
        inputPlugins = ::getInputPlugins();

        // Now grab the dynamic plugins
        for (auto loader : getLoadedPlugins()) {
            InputProvider* inputProvider = qobject_cast<InputProvider*>(loader->instance());
            if (inputProvider) {
                for (auto inputPlugin : inputProvider->getInputPlugins()) {
                    inputPlugins.push_back(inputPlugin);
                }
            }
        }

        auto& container = PluginContainer::getInstance();
        for (auto plugin : inputPlugins) {
            plugin->setContainer(&container);
            plugin->init();
        }
    });
    return inputPlugins;
}

void PluginManager::saveSettings() {
    saveInputPluginSettings(getInputPlugins());
}
