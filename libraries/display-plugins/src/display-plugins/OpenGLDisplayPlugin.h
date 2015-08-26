//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <QTimer>

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

    virtual void activate() override;
    virtual void deactivate() override;

    virtual bool eventFilter(QObject* receiver, QEvent* event) override;

    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize) override;

protected:
    virtual void customizeContext();
    virtual void drawUnitQuad();
    virtual glm::uvec2 getSurfaceSize() const = 0;
    virtual void makeCurrent() = 0;
    virtual void doneCurrent() = 0;
    virtual void swapBuffers() = 0;

    mutable QTimer _timer;
    ProgramPtr _program;
    ShapeWrapperPtr _plane;
};


