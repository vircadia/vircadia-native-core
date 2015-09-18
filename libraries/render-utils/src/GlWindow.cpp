//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GlWindow.h"

#include <QOpenGLContext>
#include <QOpenGLDebugLogger>
#include <GLHelpers.h>

GlWindow::GlWindow(QOpenGLContext* shareContext) : GlWindow(getDefaultOpenGlSurfaceFormat(), shareContext) {
}

GlWindow::GlWindow(const QSurfaceFormat& format, QOpenGLContext* shareContext) {
    setSurfaceType(QSurface::OpenGLSurface);
    setFormat(format);
    _context = new QOpenGLContext;
    _context->setFormat(format);
    if (shareContext) {
        _context->setShareContext(shareContext);
    }
    _context->create();
}

GlWindow::~GlWindow() {
    _context->doneCurrent();
    _context->deleteLater();
    _context = nullptr;
}

bool GlWindow::makeCurrent() {
    bool makeCurrentResult = _context->makeCurrent(this);
    Q_ASSERT(makeCurrentResult);
    
    std::call_once(_reportOnce, []{
        qDebug() << "GL Version: " << QString((const char*) glGetString(GL_VERSION));
        qDebug() << "GL Shader Language Version: " << QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
        qDebug() << "GL Vendor: " << QString((const char*) glGetString(GL_VENDOR));
        qDebug() << "GL Renderer: " << QString((const char*) glGetString(GL_RENDERER));
    });
    
    #ifndef QT_NO_DEBUG
    QOpenGLContext * currentContext =
    #endif
        QOpenGLContext::currentContext();
    Q_ASSERT(_context == currentContext);
    return makeCurrentResult;
}

void GlWindow::doneCurrent() {
    _context->doneCurrent();
}

void GlWindow::swapBuffers() {
    _context->swapBuffers(this);
}

