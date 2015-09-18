//
//  OffscreenGlCanvas.cpp
//  interface/src/renderer
//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "OffscreenGlCanvas.h"

#include <GLHelpers.h>
#include <QOpenGLDebugLogger>
#include <QOpenGLContext>
#include <QOffscreenSurface>

OffscreenGlCanvas::OffscreenGlCanvas() : _context(new QOpenGLContext), _offscreenSurface(new QOffscreenSurface){
}

OffscreenGlCanvas::~OffscreenGlCanvas() {
#ifdef DEBUG
    if (_logger) {
        makeCurrent();
        delete _logger;
        _logger = nullptr;
    }
#endif
    _context->doneCurrent();
}

void OffscreenGlCanvas::create(QOpenGLContext* sharedContext) {
    if (nullptr != sharedContext) {
        sharedContext->doneCurrent();
        _context->setShareContext(sharedContext);
    }
    _context->setFormat(getDefaultOpenGlSurfaceFormat());
    _context->create();

    _offscreenSurface->setFormat(_context->format());
    _offscreenSurface->create();

}

bool OffscreenGlCanvas::makeCurrent() {
    bool result = _context->makeCurrent(_offscreenSurface);
    Q_ASSERT(result);
    
    std::call_once(_reportOnce, []{
        qDebug() << "GL Version: " << QString((const char*) glGetString(GL_VERSION));
        qDebug() << "GL Shader Language Version: " << QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
        qDebug() << "GL Vendor: " << QString((const char*) glGetString(GL_VENDOR));
        qDebug() << "GL Renderer: " << QString((const char*) glGetString(GL_RENDERER));
    });

#ifdef DEBUG
    if (result && !_logger) {
        _logger = new QOpenGLDebugLogger(this);
        if (_logger->initialize()) {
            connect(_logger, &QOpenGLDebugLogger::messageLogged, [](const QOpenGLDebugMessage& message) {
                qDebug() << message;
            });
            _logger->disableMessages(QOpenGLDebugMessage::AnySource, QOpenGLDebugMessage::AnyType, QOpenGLDebugMessage::NotificationSeverity);
            _logger->startLogging(QOpenGLDebugLogger::LoggingMode::SynchronousLogging);
        }
    }
#endif

    return result;
}

void OffscreenGlCanvas::doneCurrent() {
    _context->doneCurrent();
}

