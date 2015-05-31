//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenGlDisplayPlugin.h"

#include <QOpenGLContext>
#include <TextureCache.h>
#include <PathUtils.h>

#include <QOpenGLContext>
#include <QCursor>
#include <QCoreApplication>

#include <GLWindow.h>
#include <GLMHelpers.h>


OpenGlDisplayPlugin::OpenGlDisplayPlugin() {
    connect(&_timer, &QTimer::timeout, this, [&] {
        if (_window) {
            emit requestRender();
        }
    });
}

OpenGlDisplayPlugin::~OpenGlDisplayPlugin() {
}

QWindow* OpenGlDisplayPlugin::getWindow() const {
    return _window;
}

glm::ivec2 OpenGlDisplayPlugin::getTrueMousePosition() const {
    return toGlm(_window->mapFromGlobal(QCursor::pos()));
}

QSize OpenGlDisplayPlugin::getDeviceSize() const {
    return _window->geometry().size() * _window->devicePixelRatio();
}

glm::ivec2 OpenGlDisplayPlugin::getCanvasSize() const {
    return toGlm(_window->geometry().size());
}

bool OpenGlDisplayPlugin::hasFocus() const {
    return _window->isActive();
}

void OpenGlDisplayPlugin::makeCurrent() {
    _window->makeCurrent();
}

void OpenGlDisplayPlugin::doneCurrent() {
    _window->doneCurrent();
}

void OpenGlDisplayPlugin::swapBuffers() {
    _window->swapBuffers();
}

void OpenGlDisplayPlugin::preDisplay() {
    makeCurrent();
};

void OpenGlDisplayPlugin::preRender() {
    // NOOP
}

void OpenGlDisplayPlugin::finishFrame() {
    swapBuffers();
    doneCurrent();
};


void OpenGlDisplayPlugin::activate(PluginContainer * container) {
    Q_ASSERT(nullptr == _window);
    _window = new GlWindow(QOpenGLContext::currentContext());
    _window->installEventFilter(this);
    customizeWindow(container);
//    _window->show();

    makeCurrent();
    customizeContext(container);

    _timer.start(1);
}

void OpenGlDisplayPlugin::customizeContext(PluginContainer * container) {
    using namespace oglplus;
    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    program = loadDefaultShader();
    plane = loadPlane(program);
    Context::ClearColor(0, 0, 0, 1);
    crosshairTexture = DependencyManager::get<TextureCache>()->
        getImageTexture(PathUtils::resourcesPath() + "images/sixense-reticle.png");
}

void OpenGlDisplayPlugin::deactivate() {
    _timer.stop();

    makeCurrent();
    plane.reset();
    program.reset();
    crosshairTexture.reset();
    doneCurrent();

    Q_ASSERT(nullptr != _window);
    _window->hide();
    _window->destroy();
    _window->deleteLater();
    _window = nullptr;
}

void OpenGlDisplayPlugin::installEventFilter(QObject* filter) {
    _window->installEventFilter(filter);
}

void OpenGlDisplayPlugin::removeEventFilter(QObject* filter) {
    _window->removeEventFilter(filter);
}



// Pressing Alt (and Meta) key alone activates the menubar because its style inherits the
// SHMenuBarAltKeyNavigation from QWindowsStyle. This makes it impossible for a scripts to
// receive keyPress events for the Alt (and Meta) key in a reliable manner.
//
// This filter catches events before QMenuBar can steal the keyboard focus.
// The idea was borrowed from
// http://www.archivum.info/qt-interest@trolltech.com/2006-09/00053/Re-(Qt4)-Alt-key-focus-QMenuBar-(solved).html

// Pass input events on to the application
bool OpenGlDisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseMove:
    case QEvent::Wheel:

    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::TouchUpdate:

    case QEvent::FocusIn:
    case QEvent::FocusOut:

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:

    case QEvent::DragEnter:
    case QEvent::Drop:

    case QEvent::Resize:
        if (QCoreApplication::sendEvent(QCoreApplication::instance(), event)) {
            return true;
        }
        break;
    }
    return false;
}

