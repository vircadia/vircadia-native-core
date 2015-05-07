//
//  LegacyDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Application.h"
#include "LegacyDisplayPlugin.h"
#include "MainWindow.h"
#include <RenderUtil.h>

const QString LegacyDisplayPlugin::NAME("2D Monitor (GL Windgets)");

const QString & LegacyDisplayPlugin::getName() {
    return NAME;
}

static QWidget * oldWidget = nullptr;

void LegacyDisplayPlugin::activate() {
    _window = new GLCanvas();
    QGLFormat format(QGL::NoDepthBuffer | QGL::NoStencilBuffer);
    _window->setContext(new QGLContext(format), 
        QGLContext::fromOpenGLContext(QOpenGLContext::currentContext()));
    _window->makeCurrent();
    oldWidget = qApp->getWindow()->centralWidget();
    qApp->getWindow()->setCentralWidget(_window);
    _window->doneCurrent();
    _window->setFocusPolicy(Qt::StrongFocus);
    _window->setFocus();
    _window->setMouseTracking(true);

    _window->installEventFilter(qApp);
    _window->installEventFilter(DependencyManager::get<OffscreenUi>().data());
}

void LegacyDisplayPlugin::deactivate() {
    _window->removeEventFilter(DependencyManager::get<OffscreenUi>().data());
    _window->removeEventFilter(qApp);
    qApp->getWindow()->setCentralWidget(oldWidget);
    // stop the glWidget frame timer so it doesn't call paintGL
    _window->stopFrameTimer();
    _window->doneCurrent();
    _window->deleteLater();
    _window = nullptr;
}

QSize LegacyDisplayPlugin::getDeviceSize() const {
    return _window->getDeviceSize();
}

void LegacyDisplayPlugin::makeCurrent() {
    _window->makeCurrent();
    QSize windowSize = _window->size();
    glViewport(0, 0, windowSize.width(), windowSize.height());
}

void LegacyDisplayPlugin::doneCurrent() {
    _window->doneCurrent();
}

void LegacyDisplayPlugin::swapBuffers() {
    _window->swapBuffers();
    glFinish();
}

void LegacyDisplayPlugin::idle() {
    _window->updateGL();
}

glm::ivec2 LegacyDisplayPlugin::getCanvasSize() const {
    return toGlm(_window->size());
}

bool LegacyDisplayPlugin::hasFocus() const {
    return _window->hasFocus();
}

PickRay LegacyDisplayPlugin::computePickRay(const glm::vec2 & pos) const {
    return PickRay();
}

bool isMouseOnScreen() {
    return false;
}

bool LegacyDisplayPlugin::isThrottled() {
    return _window->isThrottleRendering();
}