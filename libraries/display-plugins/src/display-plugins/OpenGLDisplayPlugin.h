//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QTimer>

#include <gpu/Texture.h>

#include "DisplayPlugin.h"
#include "OglplusHelpers.h"

class GlWindow;
class QOpenGLContext;
class OpenGLDisplayPlugin : public DisplayPlugin {
public:
    OpenGLDisplayPlugin();
    virtual ~OpenGLDisplayPlugin();
    virtual void preRender() override;
    virtual void preDisplay() override;
    virtual void finishFrame() override;

    virtual void activate(PluginContainer * container) override;
    virtual void deactivate() override;

    virtual bool eventFilter(QObject* receiver, QEvent* event) override;

    void display(
        GLuint sceneTexture, const glm::uvec2& sceneSize,
        GLuint overlayTexture, const glm::uvec2& overlaySize) override;

protected:

    // Needs to be called by the activate method after the GL context has been created to 
    // initialize OpenGL context settings needed by the plugin
    virtual void customizeContext(PluginContainer * container);

    virtual void makeCurrent() = 0;
    virtual void doneCurrent() = 0;
    virtual void swapBuffers() = 0;

    QTimer _timer;
    ProgramPtr _program;
    ShapeWrapperPtr _plane;
    gpu::TexturePointer _crosshairTexture;
};


