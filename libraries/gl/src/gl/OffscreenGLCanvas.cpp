//
//  OffscreenGLCanvas.cpp
//  interface/src/renderer
//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "OffscreenGLCanvas.h"

#include <QtCore/QProcessEnvironment>
#include <QtCore/QDebug>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>

#include "GLHelpers.h"
#include "GLLogging.h"


OffscreenGLCanvas::OffscreenGLCanvas() : _context(new QOpenGLContext), _offscreenSurface(new QOffscreenSurface){
}

OffscreenGLCanvas::~OffscreenGLCanvas() {
    // A context with logging enabled needs to be current when it's destroyed
    _context->makeCurrent(_offscreenSurface);
    delete _context;
    _context = nullptr;

    _offscreenSurface->destroy();
    delete _offscreenSurface;
    _offscreenSurface = nullptr;

}

bool OffscreenGLCanvas::create(QOpenGLContext* sharedContext) {
    if (nullptr != sharedContext) {
        sharedContext->doneCurrent();
        _context->setShareContext(sharedContext);
    }
    _context->setFormat(getDefaultOpenGLSurfaceFormat());

    if (_context->create()) {
        _offscreenSurface->setFormat(_context->format());
        _offscreenSurface->create();
        return _offscreenSurface->isValid();
    }
    qWarning("Failed to create OffscreenGLCanvas context");

    return false;
}

bool OffscreenGLCanvas::makeCurrent() {
    bool result = _context->makeCurrent(_offscreenSurface);
    Q_ASSERT(result);
    
    std::call_once(_reportOnce, [this]{
        qCDebug(glLogging) << "GL Version: " << QString((const char*) glGetString(GL_VERSION));
        qCDebug(glLogging) << "GL Shader Language Version: " << QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
        qCDebug(glLogging) << "GL Vendor: " << QString((const char*) glGetString(GL_VENDOR));
        qCDebug(glLogging) << "GL Renderer: " << QString((const char*) glGetString(GL_RENDERER));
    });

    return result;
}

void OffscreenGLCanvas::doneCurrent() {
    _context->doneCurrent();
}

QObject* OffscreenGLCanvas::getContextObject() {
    return _context;
}

void OffscreenGLCanvas::moveToThreadWithContext(QThread* thread) {
    moveToThread(thread);
    _context->moveToThread(thread);
}
