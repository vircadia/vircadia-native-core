//
//  Created by Bradley Austin Davis on 2016/08/21
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Context.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>

#ifdef Q_OS_WIN
#include <QtPlatformHeaders/QWGLNativeContext>
#endif

using namespace gl;

void Context::destroyContext(QOpenGLContext* context) {
    delete context;
}

void Context::makeCurrent(QOpenGLContext* context, QSurface* surface) {
    context->makeCurrent(surface);
}

QOpenGLContext* Context::qglContext() {
#ifdef Q_OS_WIN
    if (!_wrappedContext) {
        _wrappedContext = new QOpenGLContext();
        _wrappedContext->setNativeHandle(QVariant::fromValue(QWGLNativeContext(_hglrc, _hwnd)));
        _wrappedContext->create();
    }
    return _wrappedContext;
#else
    
    return _context;
#endif
}

void Context::moveToThread(QThread* thread) {
    qglContext()->moveToThread(thread);
}

#ifndef Q_OS_WIN
bool Context::makeCurrent() {
    return _context->makeCurrent(_window);
}

void Context::swapBuffers() {
    _context->swapBuffers(_window);
}

void Context::doneCurrent() {
    if (_context) {
        _context->doneCurrent();
    }
}

const QSurfaceFormat& getDefaultOpenGLSurfaceFormat();


void Context::create() {
    _context = new QOpenGLContext();
    if (PRIMARY) {
        _context->setShareContext(PRIMARY->qglContext());
    } else {
        PRIMARY = this;
    }
    _context->setFormat(getDefaultOpenGLSurfaceFormat());
    _context->create();
}

#endif
