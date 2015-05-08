//
//  SimpleDisplayPlugin.h
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "DisplayPlugin.h"

#include <QCursor>

#include <QOpenGLContext>
#include <GLMHelpers.h>
#include <RenderUtil.h>

class SimpleGlDisplayPlugin : public DisplayPlugin {
public:
    virtual void activate();
    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
                         GLuint overlayTexture, const glm::uvec2& overlaySize);
};

template <typename T>
class SimpleDisplayPlugin : public SimpleGlDisplayPlugin {
public:
    virtual glm::ivec2 getTrueMousePosition() const {
        return toGlm(_window->mapFromGlobal(QCursor::pos()));
    }

protected:
    T * _window;
};
