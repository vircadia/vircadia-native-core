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
        _context->setFormat(sharedContext->format());
        _context->setShareContext(sharedContext);
    } else {
        QSurfaceFormat format;
        format.setDepthBufferSize(16);
        format.setStencilBufferSize(8);
        format.setMajorVersion(4);
        format.setMinorVersion(1);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
#ifdef DEBUG
        format.setOption(QSurfaceFormat::DebugContext);
#endif
        _context->setFormat(format);
    }
    _context->create();

    _offscreenSurface->setFormat(_context->format());
    _offscreenSurface->create();

}

bool OffscreenGlCanvas::makeCurrent() {
    bool result = _context->makeCurrent(_offscreenSurface);

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

