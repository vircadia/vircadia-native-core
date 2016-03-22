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

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLDebugLogger>
#include <QtGui/QOpenGLContext>

#include "GLHelpers.h"

OffscreenGLCanvas::OffscreenGLCanvas() : _context(new QOpenGLContext), _offscreenSurface(new QOffscreenSurface){
}

OffscreenGLCanvas::~OffscreenGLCanvas() {
#ifdef DEBUG
    if (_logger) {
        makeCurrent();
        delete _logger;
        _logger = nullptr;
    }
#endif
    _context->doneCurrent();
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
        return true;
    }

    qWarning() << "Failed to create OffscreenGLCanvas";

    return false;
}

bool OffscreenGLCanvas::makeCurrent() {
    bool result = _context->makeCurrent(_offscreenSurface);
    Q_ASSERT(result);
    
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

void OffscreenGLCanvas::doneCurrent() {
    _context->doneCurrent();
}

QObject* OffscreenGLCanvas::getContextObject() {
    return _context;
}
