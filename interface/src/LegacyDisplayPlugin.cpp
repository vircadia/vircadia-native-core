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

LegacyDisplayPlugin::LegacyDisplayPlugin() {
    connect(&_timer, &QTimer::timeout, this, [&] {
        emit requestRender();
    });
}

static QWidget * oldWidget = nullptr;

void LegacyDisplayPlugin::activate(PluginContainer * container) {
    _window = new GLCanvas();
    QOpenGLContext * sourceContext = QOpenGLContext::currentContext();
    QOpenGLContext * newContext = new QOpenGLContext();
    {
        QSurfaceFormat format;
        format.setOption(QSurfaceFormat::DebugContext);
        newContext->setFormat(format);
    }

    _window->setContext(
        QGLContext::fromOpenGLContext(newContext),
        QGLContext::fromOpenGLContext(sourceContext));
    oldWidget = qApp->getWindow()->centralWidget();

    // FIXME necessary?
    makeCurrent();
    qApp->getWindow()->setCentralWidget(_window);
    doneCurrent();
    _window->setFocusPolicy(Qt::StrongFocus);
    _window->setFocus();
    _window->setMouseTracking(true);
    _timer.start(8);
}

void LegacyDisplayPlugin::deactivate() {
    _timer.stop();
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

void LegacyDisplayPlugin::preDisplay() {
    OpenGlDisplayPlugin::preDisplay();
    auto size = toGlm(_window->size());
    glViewport(0, 0, size.x, size.y);
}

bool LegacyDisplayPlugin::isThrottled() const {
    return _window->isThrottleRendering();
}

void LegacyDisplayPlugin::makeCurrent() {
    _window->makeCurrent();
}

void LegacyDisplayPlugin::doneCurrent() {
    _window->doneCurrent();
}

void LegacyDisplayPlugin::swapBuffers() {
    _window->swapBuffers();
}

ivec2 LegacyDisplayPlugin::getTrueMousePosition() const {
    return toGlm(_window->mapFromGlobal(QCursor::pos()));
}

QWindow* LegacyDisplayPlugin::getWindow() const {
    return _window->windowHandle();
}

void LegacyDisplayPlugin::installEventFilter(QObject* filter) {
    _window->installEventFilter(filter);
}

void LegacyDisplayPlugin::removeEventFilter(QObject* filter) {
    _window->removeEventFilter(filter);
}