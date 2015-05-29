//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GlWindowDisplayPlugin.h"

#include <QOpenGLContext>
#include <QCursor>
#include <QTimer>
#include <QCoreApplication>

#include <GLWindow.h>
#include <GLMHelpers.h>

GlWindowDisplayPlugin::GlWindowDisplayPlugin() : _timer(new QTimer()) {
    connect(_timer, &QTimer::timeout, this, [&] {
        //        if (qApp->getActiveDisplayPlugin() == this) {
        emit requestRender();
        //        }
    });
}

GlWindowDisplayPlugin::~GlWindowDisplayPlugin() {
    delete _timer;
}

void GlWindowDisplayPlugin::makeCurrent() {
    _window->makeCurrent();
}

void GlWindowDisplayPlugin::doneCurrent() {
    _window->doneCurrent();
}

void GlWindowDisplayPlugin::swapBuffers() {
    _window->swapBuffers();
}

glm::ivec2 GlWindowDisplayPlugin::getTrueMousePosition() const {
    return toGlm(_window->mapFromGlobal(QCursor::pos()));
}

QWindow* GlWindowDisplayPlugin::getWindow() const {
    return _window;
}

bool GlWindowDisplayPlugin::eventFilter(QObject* object, QEvent* event) {
    if (qApp->eventFilter(object, event)) {
        return true;
    }

    // FIXME
    /*
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    if (offscreenUi->eventFilter(object, event)) {
        return true;
    }
    */

    // distinct calls for easier debugging with breakpoints
    switch (event->type()) {
        case QEvent::KeyPress:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        case QEvent::KeyRelease:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        case QEvent::MouseButtonPress:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        case QEvent::MouseButtonRelease:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        case QEvent::FocusIn:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        case QEvent::FocusOut:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        case QEvent::Resize:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        case QEvent::MouseMove:
            QCoreApplication::sendEvent(QCoreApplication::instance(), event);
            break;
        default:
            break;
    }
    return false;
}

void GlWindowDisplayPlugin::activate(PluginContainer * container) {
    Q_ASSERT(nullptr == _window);
    _window = new GlWindow(QOpenGLContext::currentContext());
    _window->installEventFilter(this);
    customizeWindow();
    _window->show();
    _timer->start(8);
    makeCurrent();
    customizeContext();
    // FIXME
    //DependencyManager::get<OffscreenUi>()->setProxyWindow(_window);
}

void GlWindowDisplayPlugin::deactivate() {
    _timer->stop();
    Q_ASSERT(nullptr != _window);
    _window->hide();
    _window->destroy();
    _window->deleteLater();
    _window = nullptr;
}

QSize GlWindowDisplayPlugin::getDeviceSize() const {
    return _window->geometry().size() * _window->devicePixelRatio();
}

glm::ivec2 GlWindowDisplayPlugin::getCanvasSize() const {
    return toGlm(_window->geometry().size());
}

bool GlWindowDisplayPlugin::hasFocus() const {
    return _window->isActive();
}
