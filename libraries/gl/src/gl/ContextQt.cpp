//
//  Created by Bradley Austin Davis on 2016/08/21
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Note, `gl::Context is split into two files because a single file cannot include both the GLAD headers 
// and the QOpenGLContext definition headers

#include "Context.h"

#include "Config.h"

#include <QtGui/QWindow>
#include "QOpenGLContextWrapper.h"

#ifdef Q_OS_WIN
#include <QtPlatformHeaders/QWGLNativeContext>
#endif

#include <QtGui/QOpenGLDebugMessage>

#include "GLHelpers.h"

using namespace gl;

void Context::destroyContext(QOpenGLContext* context) {
    context->deleteLater();
}

void Context::makeCurrent(QOpenGLContext* context, QSurface* surface) {
    context->makeCurrent(surface);
}

#if defined(GL_CUSTOM_CONTEXT)
void Context::createWrapperContext() {
    if (0 != _hglrc && nullptr == _qglContext) {
        _qglContext = new QOpenGLContext();
        _qglContext->setNativeHandle(QVariant::fromValue(QWGLNativeContext(_hglrc, _hwnd)));
        _qglContext->create();
    }
}
#endif


void Context::moveToThread(QThread* thread) {
    if (_qglContext) {
        _qglContext->moveToThread(thread);
    }
}

void Context::debugMessageHandler(const QOpenGLDebugMessage& debugMessage) {
    auto severity = debugMessage.severity();
    switch (severity) {
        case QOpenGLDebugMessage::NotificationSeverity:
        case QOpenGLDebugMessage::LowSeverity:
            return;
        default:
            break;
    }
    qDebug(glLogging) << debugMessage;
    return;
}

void Context::setupDebugLogging(QOpenGLContext *context) {
    QOpenGLDebugLogger *logger = new QOpenGLDebugLogger(context);
    QObject::connect(logger, &QOpenGLDebugLogger::messageLogged, nullptr, [](const QOpenGLDebugMessage& message){
        Context::debugMessageHandler(message);
    });
    if (logger->initialize()) {
        logger->enableMessages();
        logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
    } else {
        qCWarning(glLogging) <<  "OpenGL context does not support debugging";
    }
}

bool Context::makeCurrent() {
    updateSwapchainMemoryCounter();
    bool result = _qglContext->makeCurrent(_window);
    gl::initModuleGl();
    return result;
}

void Context::swapBuffers() {
    qglContext()->swapBuffers(_window);
}

void Context::doneCurrent() {
    if (_qglContext) {
        _qglContext->doneCurrent();
    }
}

Q_GUI_EXPORT QOpenGLContext *qt_gl_global_share_context();
const QSurfaceFormat& getDefaultOpenGLSurfaceFormat();

void Context::qtCreate(QOpenGLContext* shareContext) {
    _qglContext = new QOpenGLContext();
    _qglContext->setFormat(_window->format());
    if (!shareContext) {
        shareContext = qt_gl_global_share_context();
    }

    if (shareContext) {
        _qglContext->setShareContext(shareContext);
    }
    _qglContext->create();
    _swapchainPixelSize = evalGLFormatSwapchainPixelSize(_qglContext->format());
}
