//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLWindow.h"

#include "Config.h"
#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>

#include "GLHelpers.h"
#include "GLLogging.h"

void GLWindow::createContext(QOpenGLContext* shareContext) {
    createContext(getDefaultOpenGLSurfaceFormat(), shareContext);
}

void GLWindow::createContext(const QSurfaceFormat& format, QOpenGLContext* shareContext) {
    _context = new gl::Context();
    _context->setWindow(this);
    _context->create(shareContext);
    _context->makeCurrent();
    _context->clear();
}

GLWindow::~GLWindow() {
}

bool GLWindow::makeCurrent() {
    bool makeCurrentResult = _context->makeCurrent();
    Q_ASSERT(makeCurrentResult);

    if (makeCurrentResult) {
        std::call_once(_reportOnce, []{
            LOG_GL_CONTEXT_INFO(glLogging, gl::ContextInfo().init());
        });
    }
    return makeCurrentResult;
}

void GLWindow::doneCurrent() {
    _context->doneCurrent();
}

void GLWindow::swapBuffers() {
    _context->swapBuffers();
}

QOpenGLContext* GLWindow::context() const {
    return _context->qglContext();
}


