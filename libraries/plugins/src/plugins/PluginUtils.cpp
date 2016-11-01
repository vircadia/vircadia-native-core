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
        }
    }
    return false;
}

bool PluginUtils::isHandControllerAvailable() {
    for (auto& inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->isHandController()) {
            return true;
        }
    }
    return false;
};

bool isSubdeviceContainingNameAvailable(QString name) {
    for (auto& inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        auto subdeviceNames = inputPlugin->getSubdeviceNames();
        for (auto& name : subdeviceNames) {
            if (name.contains(name)) {
                return true;
            }
        }
    }
    return false;
};

bool PluginUtils::isViveControllerAvailable() {
    return isSubdeviceContainingNameAvailable("Vive");
};

bool PluginUtils::isXboxControllerAvailable() {
    return isSubdeviceContainingNameAvailable("X360 Controller");
};

