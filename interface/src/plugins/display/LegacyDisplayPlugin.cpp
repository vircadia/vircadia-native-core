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
#include <QOpenGLDebugLogger>

const QString LegacyDisplayPlugin::NAME("2D Monitor (GL Windgets)");

const QString & LegacyDisplayPlugin::getName() {
    return NAME;
}

static QWidget * oldWidget = nullptr;

void LegacyDisplayPlugin::activate() {
    _window = new GLCanvas();

    QOpenGLContext * sourceContext = QOpenGLContext::currentContext();
    QSurfaceFormat format;
    format.setOption(QSurfaceFormat::DebugContext);


    QOpenGLContext * newContext = new QOpenGLContext();
    newContext->setFormat(format);
    _window->setContext(
        QGLContext::fromOpenGLContext(newContext),
        QGLContext::fromOpenGLContext(sourceContext));
    _window->makeCurrent();

    oldWidget = qApp->getWindow()->centralWidget();
    qApp->getWindow()->setCentralWidget(_window);
    _window->doneCurrent();
    _window->setFocusPolicy(Qt::StrongFocus);
    _window->setFocus();
    _window->setMouseTracking(true);

    _window->installEventFilter(qApp);
    _window->installEventFilter(DependencyManager::get<OffscreenUi>().data());

    DependencyManager::get<OffscreenUi>()->setProxyWindow(_window->windowHandle());
    SimpleDisplayPlugin::activate();
}

void LegacyDisplayPlugin::deactivate() {
    _window->removeEventFilter(DependencyManager::get<OffscreenUi>().data());
    _window->removeEventFilter(qApp);
    // FIXME, during shutdown, this causes an NPE.  Need to deactivate the plugin before the main window is destroyed.
//    if (qApp->getWindow()) {
//        qApp->getWindow()->setCentralWidget(oldWidget);
//    }
    // stop the glWidget frame timer so it doesn't call paintGL
    _window->stopFrameTimer();
    _window->doneCurrent();
    _window->deleteLater();
    _window = nullptr;
}

QSize LegacyDisplayPlugin::getDeviceSize() const {
    return _window->getDeviceSize();
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

void LegacyDisplayPlugin::preDisplay() {
    SimpleDisplayPlugin::preDisplay();
    auto size = toGlm(_window->size());
    glViewport(0, 0, size.x, size.y);
}

bool LegacyDisplayPlugin::isThrottled() const {
    return _window->isThrottleRendering();
}