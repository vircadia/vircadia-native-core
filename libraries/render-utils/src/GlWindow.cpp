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
#ifdef DEBUG
    if (_logger) {
        makeCurrent();
        delete _logger;
        _logger = nullptr;
    }
#endif
    _context->doneCurrent();
    _context->deleteLater();
    _context = nullptr;
}


void GlWindow::makeCurrent() {
	_context->makeCurrent(this);
#ifdef DEBUG
  if (!_logger) {
      _logger = new QOpenGLDebugLogger(this);
      if (_logger->initialize()) {
          connect(_logger, &QOpenGLDebugLogger::messageLogged, [](const QOpenGLDebugMessage& message) {
              if (message.type() == QOpenGLDebugMessage::Type::ErrorType) {
                  qDebug() << "Error";
              }
              qDebug() << message;
          });
          _logger->disableMessages(QOpenGLDebugMessage::AnySource, QOpenGLDebugMessage::AnyType, QOpenGLDebugMessage::NotificationSeverity);
          _logger->startLogging(QOpenGLDebugLogger::LoggingMode::SynchronousLogging);
      }
  }
#endif
}

void GlWindow::doneCurrent() {
	_context->doneCurrent();
}

void GlWindow::swapBuffers() {
	_context->swapBuffers(this);
}

