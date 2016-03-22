//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLWindow.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

#include "GLHelpers.h"

void GLWindow::createContext(QOpenGLContext* shareContext) {
    createContext(getDefaultOpenGLSurfaceFormat(), shareContext);
}

void GLWindow::createContext(const QSurfaceFormat& format, QOpenGLContext* shareContext) {
    setSurfaceType(QSurface::OpenGLSurface);
    setFormat(format);
    _context = new QOpenGLContext;
    _context->setFormat(format);
    if (shareContext) {
        _context->setShareContext(shareContext);
    }
    _context->create();
}

GLWindow::~GLWindow() {
    if (_context) {
        _context->doneCurrent();
        _context->deleteLater();
        _context = nullptr;
    }
}

bool GLWindow::makeCurrent() {
    bool makeCurrentResult = _context->makeCurrent(this);
    Q_ASSERT(makeCurrentResult);
    Q_ASSERT(_context == QOpenGLContext::currentContext());
    
    return makeCurrentResult;
}

void GLWindow::doneCurrent() {
    _context->doneCurrent();
}

void GLWindow::swapBuffers() {
    _context->swapBuffers(this);
}

QOpenGLContext* GLWindow::context() const {
    return _context;
}


