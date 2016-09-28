//  PluginUtils.cpp
//  input-plugins/src/input-plugins
//
//  Created by Ryan Huffman on 9/22/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PluginUtils.h"

#include "DisplayPlugin.h"
#include "InputPlugin.h"
#include "PluginManager.h"

bool PluginUtils::isHMDAvailable(const QString& pluginName) {
    for (auto& displayPlugin : PluginManager::getInstance()->getDisplayPlugins()) {
        // Temporarily only enable this for Vive
        if (displayPlugin->isHmd() && (pluginName.isEmpty() || displayPlugin->getName() == pluginName)) {
            return true;
            break;
        }
    }
    return false;
}

bool PluginUtils::isHandControllerAvailable() {
    for (auto& inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->isHandController()) {
            return true;
            break;
        }
    }
    return false;
};
