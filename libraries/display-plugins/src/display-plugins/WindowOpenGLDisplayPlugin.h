//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "OpenGLDisplayPlugin.h"

class QGLWidget;

class WindowOpenGLDisplayPlugin : public OpenGLDisplayPlugin {
public:
    WindowOpenGLDisplayPlugin();
    virtual glm::uvec2 getRecommendedRenderSize() const override;
    virtual glm::uvec2 getRecommendedUiSize() const override;
    virtual bool hasFocus() const override;
    virtual void activate() override;
    virtual void deactivate() override;

protected:
    virtual void makeCurrent() override;
    virtual void doneCurrent() override;
    virtual void swapBuffers() override;
    QGLWidget* _window{ nullptr };
};
