//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QtGui/QWindow>
#include <QtCore/QTime>

#include <GLMHelpers.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gpu/Forward.h>
#include "TestHelpers.h"

#define DEFERRED_LIGHTING

class TestWindow : public QWindow {
protected:
    QOpenGLContextWrapper _glContext;
    QSize _size;
    glm::mat4 _projectionMatrix;
    bool _aboutToQuit { false };
    std::shared_ptr<RenderArgs> _renderArgs{ std::make_shared<RenderArgs>() };

    TestWindow();
    virtual void initGl();
    virtual void renderFrame() = 0;

private:
    void resizeWindow(const QSize& size);

    void beginFrame();
    void endFrame();
    void draw();
    void resizeEvent(QResizeEvent* ev) override;
};

