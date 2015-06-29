//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "WindowOpenGLDisplayPlugin.h"

#include <GlWindow.h>
#include <QOpenGLContext>

WindowOpenGLDisplayPlugin::WindowOpenGLDisplayPlugin() {
}

glm::uvec2 WindowOpenGLDisplayPlugin::getRecommendedRenderSize() const {
    return toGlm(_window->geometry().size() * _window->devicePixelRatio());
}

glm::uvec2 WindowOpenGLDisplayPlugin::getRecommendedUiSize() const {
    return toGlm(_window->geometry().size());
}

bool WindowOpenGLDisplayPlugin::hasFocus() const {
    return _window->isActive();
}

void WindowOpenGLDisplayPlugin::initSurfaceFormat(QSurfaceFormat& format) {
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(0);
    format.setStencilBufferSize(0);
    format.setVersion(4, 1);
#ifdef DEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
}

void WindowOpenGLDisplayPlugin::activate(PluginContainer * container) {
    OpenGLDisplayPlugin::activate(container);
    _window = createWindow(container);

    QSurfaceFormat format;
    initSurfaceFormat(format);
    _window->setFormat(format);
    _window->create();
    _window->installEventFilter(this);
    customizeWindow(container);

    makeCurrent();
    customizeContext(container);
}

void WindowOpenGLDisplayPlugin::deactivate() {
    OpenGLDisplayPlugin::deactivate();
    destroyWindow();
}

GlWindow* WindowOpenGLDisplayPlugin::createWindow(PluginContainer * container) {
    return new GlWindow(QOpenGLContext::currentContext());
}

void WindowOpenGLDisplayPlugin::destroyWindow() {
    _window->deleteLater();
    _window = nullptr;
}


void WindowOpenGLDisplayPlugin::makeCurrent() {
    bool makeCurrentResult = _window->makeCurrent();
    if (!makeCurrentResult) {
        qDebug() << "Failed to make current";
    }
}

void WindowOpenGLDisplayPlugin::doneCurrent() {
    _window->doneCurrent();
}

void WindowOpenGLDisplayPlugin::swapBuffers() {
    _window->swapBuffers();
}

void WindowOpenGLDisplayPlugin::installEventFilter(QObject* filter) {
    _window->installEventFilter(filter);
}

void WindowOpenGLDisplayPlugin::removeEventFilter(QObject* filter) {
    _window->removeEventFilter(filter);
}

QWindow* WindowOpenGLDisplayPlugin::getWindow() const {
    return _window;
}
