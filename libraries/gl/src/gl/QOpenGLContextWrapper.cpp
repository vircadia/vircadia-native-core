//
//  QOpenGLContextWrapper.cpp
//
//
//  Created by Clement on 12/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QOpenGLContextWrapper.h"

#include <QOpenGLContext>

uint32_t QOpenGLContextWrapper::currentContextVersion() {
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        return 0;
    }
    auto format = context->format();
    auto version = (format.majorVersion() << 8) + format.minorVersion();
    return version;
}


QOpenGLContext* QOpenGLContextWrapper::currentContext() {
    return QOpenGLContext::currentContext();
}

QOpenGLContextWrapper::QOpenGLContextWrapper() :
    _ownContext(true), _context(new QOpenGLContext) { }

QOpenGLContextWrapper::QOpenGLContextWrapper(QOpenGLContext* context) :
    _context(context) { }

QOpenGLContextWrapper::~QOpenGLContextWrapper() {
    if (_ownContext) {
        delete _context;
        _context = nullptr;
    }
}

void QOpenGLContextWrapper::setFormat(const QSurfaceFormat& format) {
    _context->setFormat(format);
}

bool QOpenGLContextWrapper::create() {
    return _context->create();
}

void QOpenGLContextWrapper::swapBuffers(QSurface* surface) {
    _context->swapBuffers(surface);
}

bool QOpenGLContextWrapper::makeCurrent(QSurface* surface) {
    return _context->makeCurrent(surface);
}

void QOpenGLContextWrapper::doneCurrent() {
    _context->doneCurrent();
}

void QOpenGLContextWrapper::setShareContext(QOpenGLContext* otherContext) {
    _context->setShareContext(otherContext);
}

bool isCurrentContext(QOpenGLContext* context) {
    return QOpenGLContext::currentContext() == context;
}

void QOpenGLContextWrapper::moveToThread(QThread* thread) {
    _context->moveToThread(thread);
}

