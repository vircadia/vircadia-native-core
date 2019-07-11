//
//  Created by Bradley Austin Davis on 2015/10/04
//  Copyright 2013-2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AbstractHMDScriptingInterface.h"

#include <SettingHandle.h>

#include "DisplayPlugin.h"
#include <ui-plugins/PluginContainer.h>

static Setting::Handle<float> IPD_SCALE_HANDLE("hmd.ipdScale", 1.0f);

AbstractHMDScriptingInterface::AbstractHMDScriptingInterface() {
    _IPDScale = IPD_SCALE_HANDLE.get();
}

float AbstractHMDScriptingInterface::getIPD() const {
    return PluginContainer::getInstance().getActiveDisplayPlugin()->getIPD();
}

float AbstractHMDScriptingInterface::getEyeHeight() const {
    // FIXME update the display plugin interface to expose per-plugin settings
    return getPlayerHeight() - 0.10f;
}

float AbstractHMDScriptingInterface::getPlayerHeight() const {
    // FIXME update the display plugin interface to expose per-plugin settings
    return 1.755f;
}

float AbstractHMDScriptingInterface::getIPDScale() const {
    return _IPDScale;
}

void AbstractHMDScriptingInterface::setIPDScale(float IPDScale) {
    IPDScale = glm::clamp(IPDScale, -1.0f, 3.0f);
    if (IPDScale != _IPDScale) {
        _IPDScale = IPDScale;
        IPD_SCALE_HANDLE.set(IPDScale);
        emit IPDScaleChanged();
    }
}

bool AbstractHMDScriptingInterface::isHMDMode() const {
    auto displayPlugin = PluginContainer::getInstance().getActiveDisplayPlugin();
    if (displayPlugin) {
        return displayPlugin->isHmd();
    }
    return false;
}