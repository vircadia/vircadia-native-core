//
//  InputPlugin.h
//  input-plugins/src/input-plugins
//
//  Created by Sam Gondelman on 7/13/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "Plugin.h"

namespace controller {
    struct InputCalibrationData;
}

class InputPlugin : public Plugin {
public:
    virtual void pluginFocusOutEvent() = 0;
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) = 0;

    // Some input plugins are comprised of multiple subdevices (SDL2, for instance).
    // If an input plugin is only a single device, it will only return it's primary name.
    virtual QStringList getSubdeviceNames() { return { getName() }; };
    virtual bool isHandController() const = 0;
};

