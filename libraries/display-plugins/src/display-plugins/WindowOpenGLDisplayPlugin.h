//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OpenGLDisplayPlugin.h"

class GlWindow;
class QSurfaceFormat;

class WindowOpenGLDisplayPlugin : public OpenGLDisplayPlugin {
    Q_OBJECT
public:
    WindowOpenGLDisplayPlugin();

    virtual glm::uvec2 getRecommendedRenderSize() const override;
    virtual glm::uvec2 getRecommendedUiSize() const override;
    virtual bool hasFocus() const override;
    virtual QWindow* getWindow() const override;
    virtual void activate(PluginContainer * container) override;
    virtual void deactivate() override;
    virtual void installEventFilter(QObject* filter) override;
    virtual void removeEventFilter(QObject* filter) override;

protected:
    virtual GlWindow* createWindow(PluginContainer * container);
    virtual void customizeWindow(PluginContainer * container) = 0;

    virtual void destroyWindow();

    virtual void makeCurrent() override;
    virtual void doneCurrent() override;
    virtual void swapBuffers() override;
    virtual void initSurfaceFormat(QSurfaceFormat& format);

    GlWindow* _window{ nullptr };
};
