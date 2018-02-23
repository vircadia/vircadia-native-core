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

#include "Config.h"

#include <QtCore/QProcessEnvironment>
#include <QtCore/QDebug>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLDebugLogger>

#include "Context.h"
#include "GLHelpers.h"
#include "GLLogging.h"


OffscreenGLCanvas::OffscreenGLCanvas() :
    _context(new QOpenGLContext),
    _offscreenSurface(new QOffscreenSurface)
{
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
    if (!_context->create()) {
        qFatal("Failed to create OffscreenGLCanvas context");
    }

    _offscreenSurface->setFormat(_context->format());
    _offscreenSurface->create();

    // Due to a https://bugreports.qt.io/browse/QTBUG-65125 we can't rely on `isValid`
    // to determine if the offscreen surface was successfully created, so we use
    // makeCurrent as a proxy test.  Bug is fixed in Qt 5.9.4
#if defined(Q_OS_ANDROID)
    if (!_context->makeCurrent(_offscreenSurface)) {
        qFatal("Unable to make offscreen surface current");
    }
#else
    if (!_offscreenSurface->isValid()) {
        qFatal("Offscreen surface is invalid");
    }
#endif

    if (gl::Context::enableDebugLogger()) {
        _context->makeCurrent(_offscreenSurface);
        QOpenGLDebugLogger *logger = new QOpenGLDebugLogger(this);
        connect(logger, &QOpenGLDebugLogger::messageLogged, this, &OffscreenGLCanvas::onMessageLogged);
        logger->initialize();
        logger->enableMessages();
        logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        _context->doneCurrent();
    }

    return true;
}

void OffscreenGLCanvas::onMessageLogged(const QOpenGLDebugMessage& debugMessage) {
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

bool OffscreenGLCanvas::makeCurrent() {
    bool result = _context->makeCurrent(_offscreenSurface);
    std::call_once(_reportOnce, []{
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
