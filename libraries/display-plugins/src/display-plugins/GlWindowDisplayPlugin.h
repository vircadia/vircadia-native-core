//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OpenGlDisplayPlugin.h"

class GlWindow;
class QTimer;

class GlWindowDisplayPlugin : public OpenGlDisplayPlugin {
public:
    GlWindowDisplayPlugin();
    virtual ~GlWindowDisplayPlugin();
    virtual void activate(PluginContainer * container);
    virtual void deactivate();
    virtual QSize getDeviceSize() const final;
    virtual glm::ivec2 getCanvasSize() const final;
    virtual bool hasFocus() const;
    virtual bool eventFilter(QObject* object, QEvent* event);

    virtual glm::ivec2 getTrueMousePosition() const;
    virtual QWindow* getWindow() const;

protected:
    virtual void makeCurrent() final;
    virtual void doneCurrent() final;
    virtual void swapBuffers() final;
    virtual void customizeWindow() = 0;
    virtual void customizeContext() = 0;

private:
    QTimer* _timer{ nullptr };
protected:
    GlWindow* _window{ nullptr };
};

