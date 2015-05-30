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

void GlWindowDisplayPlugin::activate(PluginContainer * container) {
    Q_ASSERT(nullptr == _window);
    _window = new GlWindow(QOpenGLContext::currentContext());
    _window->installEventFilter(this);
    customizeWindow();
    _window->show();
    _timer->start(8);
    makeCurrent();
    customizeContext();
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

void GlWindowDisplayPlugin::installEventFilter(QObject* filter) {
    _window->installEventFilter(filter);
}

void GlWindowDisplayPlugin::removeEventFilter(QObject* filter) {
    _window->removeEventFilter(filter);
}

bool GlWindowDisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Resize:
        case QEvent::TouchBegin:
        case QEvent::TouchEnd:
        case QEvent::TouchUpdate:
        case QEvent::Wheel:
        case QEvent::DragEnter:
        case QEvent::Drop:
            if (QCoreApplication::sendEvent(QCoreApplication::instance(), event)) {
                return true;
            }
            break;
    }
    return false;
}