//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenGLDisplayPlugin.h"
#include <QOpenGLContext>
#include <QCoreApplication>

#include <GlWindow.h>
#include <GLMHelpers.h>


OpenGLDisplayPlugin::OpenGLDisplayPlugin() {
    connect(&_timer, &QTimer::timeout, this, [&] {
        if (_active) {
            emit requestRender();
        }
    });
}

OpenGLDisplayPlugin::~OpenGLDisplayPlugin() {
}

void OpenGLDisplayPlugin::preDisplay() {
    makeCurrent();
};

void OpenGLDisplayPlugin::preRender() {
    // NOOP
}

void OpenGLDisplayPlugin::finishFrame() {
    swapBuffers();
    doneCurrent();
};

void OpenGLDisplayPlugin::customizeContext() {
    using namespace oglplus;
    // TODO: write the poper code for linux
#if defined(Q_OS_WIN)
    _vsyncSupported = wglewGetExtension("WGL_EXT_swap_control");
#endif

    Context::BlendFunc(BlendFunction::SrcAlpha, BlendFunction::OneMinusSrcAlpha);
    Context::Disable(Capability::Blend);
    Context::Disable(Capability::DepthTest);
    Context::Disable(Capability::CullFace);
    
    
    _program = loadDefaultShader();
    _plane = loadPlane(_program);

    enableVsync();
}

void OpenGLDisplayPlugin::activate() {
    _active = true;
    _timer.start(1);
}

void OpenGLDisplayPlugin::stop() {
    _active = false;
    _timer.stop();
}

void OpenGLDisplayPlugin::deactivate() {
    _active = false;
    _timer.stop();

    makeCurrent();
    Q_ASSERT(0 == glGetError());
    _program.reset();
    _plane.reset();
    doneCurrent();
}

// Pressing Alt (and Meta) key alone activates the menubar because its style inherits the
// SHMenuBarAltKeyNavigation from QWindowsStyle. This makes it impossible for a scripts to
// receive keyPress events for the Alt (and Meta) key in a reliable manner.
//
// This filter catches events before QMenuBar can steal the keyboard focus.
// The idea was borrowed from
// http://www.archivum.info/qt-interest@trolltech.com/2006-09/00053/Re-(Qt4)-Alt-key-focus-QMenuBar-(solved).html

// Pass input events on to the application
bool OpenGLDisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
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
        default:
            break;
    }
    return false;
}

void OpenGLDisplayPlugin::display(
    GLuint finalTexture, const glm::uvec2& sceneSize) {
    using namespace oglplus;
    uvec2 size = getSurfaceSize();
    Context::Viewport(size.x, size.y);
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    drawUnitQuad();
}

void OpenGLDisplayPlugin::drawUnitQuad() {
    _program->Bind();
    _plane->Draw();
}

void OpenGLDisplayPlugin::enableVsync(bool enable) {
    if (!_vsyncSupported) {
        return;
    }
#ifdef Q_OS_WIN
    wglSwapIntervalEXT(enable ? 1 : 0);
#endif
}

bool OpenGLDisplayPlugin::isVsyncEnabled() {
    if (!_vsyncSupported) {
        return true;
    }
#ifdef Q_OS_WIN
    return wglGetSwapIntervalEXT() != 0;
#else
    return true;
#endif
}
