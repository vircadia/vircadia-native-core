//
//  Created by Sam Gondelman on 7/13/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <plugins/Plugin.h>

const float DEFAULT_HAND_RETICLE_MOVE_SPEED = 37.5f;

class InputPlugin : public Plugin {
public:
    virtual bool isHandController() const = 0;

    virtual void pluginFocusOutEvent() = 0;

    virtual void pluginUpdate(float deltaTime) = 0;
};

