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
    for (const auto& displayPlugin : PluginManager::getInstance()->getDisplayPlugins()) {
        // Temporarily only enable this for Vive
        if (displayPlugin->isHmd() && (pluginName.isEmpty() || displayPlugin->getName() == pluginName)) {
            return true;
        }
    }
    return false;
}

bool PluginUtils::isHeadControllerAvailable(const QString& pluginName) {
    for (const auto& inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->isHeadController() && (pluginName.isEmpty() || inputPlugin->getName() == pluginName)) {
            return true;
        }
    }
    return false;
};

bool PluginUtils::isHandControllerAvailable(const QString& pluginName) {
    for (const auto& inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->isHandController() && (pluginName.isEmpty() || inputPlugin->getName() == pluginName)) {
            return true;
        }
    }
    return false;
};

bool PluginUtils::isSubdeviceContainingNameAvailable(QString name) {
    for (const auto& inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->isActive()) {
            auto subdeviceNames = inputPlugin->getSubdeviceNames();
            for (auto& subdeviceName : subdeviceNames) {
                if (subdeviceName.contains(name)) {
                    return true;
                }
            }
        }
    }
    return false;
};

bool PluginUtils::isViveControllerAvailable() {
    return isSubdeviceContainingNameAvailable("OpenVR");
};

bool PluginUtils::isOculusTouchControllerAvailable() {
    return isSubdeviceContainingNameAvailable("OculusTouch");
};

bool PluginUtils::isXboxControllerAvailable() {
    return isSubdeviceContainingNameAvailable("X360 Controller") || isSubdeviceContainingNameAvailable("XInput Controller");
};

