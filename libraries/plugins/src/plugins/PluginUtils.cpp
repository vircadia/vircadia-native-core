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

bool PluginUtils::isHMDAvailable() {
    for (auto& displayPlugin : PluginManager::getInstance()->getDisplayPlugins()) {
        if (displayPlugin->isHmd()) {
            return true;
            break;
        }
    }
    return false;
}

bool PluginUtils::isHandControllerAvailable() {
    for (auto& inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->getName() == "OpenVR") {
            return true;
            break;
        }
    }
    return false;
};
