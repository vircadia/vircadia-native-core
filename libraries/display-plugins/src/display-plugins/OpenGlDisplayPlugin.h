//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "DisplayPlugin.h"

class OpenGlDisplayPlugin : public DisplayPlugin {
public:
    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
                         GLuint overlayTexture, const glm::uvec2& overlaySize);
    virtual void preRender();
    virtual void preDisplay();
    virtual void finishFrame();

protected:
    virtual void makeCurrent() = 0;
    virtual void doneCurrent() = 0;
    virtual void swapBuffers() = 0;
};
