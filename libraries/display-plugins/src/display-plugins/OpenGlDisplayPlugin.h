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
#include "../OglplusHelpers.h"

class GlWindow;

class OpenGlDisplayPlugin : public DisplayPlugin {
public:
    OpenGlDisplayPlugin();
    virtual ~OpenGlDisplayPlugin();
    virtual void preRender();
    virtual void preDisplay();
    virtual void finishFrame();

    virtual void activate(PluginContainer * container);
    virtual void deactivate();

    virtual QSize getDeviceSize() const;
    virtual glm::ivec2 getCanvasSize() const;
    virtual bool hasFocus() const;

    virtual glm::ivec2 getTrueMousePosition() const;
    virtual QWindow* getWindow() const;
    virtual bool eventFilter(QObject* receiver, QEvent* event);
    virtual void installEventFilter(QObject* filter);
    virtual void removeEventFilter(QObject* filter);


protected:
    // Called by the activate method to specify the initial 
    // window geometry flags, etc
    virtual void customizeWindow(PluginContainer * container) = 0;

    // Needs to be called by the activate method after the GL context has been created to 
    // initialize OpenGL context settings needed by the plugin
    virtual void customizeContext(PluginContainer * container);

    virtual void makeCurrent() final;
    virtual void doneCurrent() final;
    virtual void swapBuffers() final;

    GlWindow* _window{ nullptr };
    QTimer _timer;
    ProgramPtr program;
    ShapeWrapperPtr plane;
    gpu::TexturePointer crosshairTexture;
};


