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

static QSurfaceFormat getDefaultFormat() {
    QSurfaceFormat format;
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setVersion(4, 1);
#ifdef DEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
    return format;
}

GlWindow::GlWindow(QOpenGLContext * shareContext) : GlWindow(getDefaultFormat(), shareContext) {
}

GlWindow::GlWindow(const QSurfaceFormat& format, QOpenGLContext * shareContext) {
    setSurfaceType(QSurface::OpenGLSurface);
    setFormat(format);
    _context = new QOpenGLContext;
    _context->setFormat(format);
    if (shareContext) {
        _context->setShareContext(shareContext);
    }
    _context->create();
}

static QOpenGLDebugLogger* logger{ nullptr };

GlWindow::~GlWindow() {
    if (logger) {
        makeCurrent();
        delete logger;
        logger = nullptr;
    }
    _context->doneCurrent();
    _context->deleteLater();
    _context = nullptr;
}


void GlWindow::makeCurrent() {
	_context->makeCurrent(this);
#ifdef DEBUG
  if (!logger) {
      logger = new QOpenGLDebugLogger(this);
      if (logger->initialize()) {
          connect(logger, &QOpenGLDebugLogger::messageLogged, [](const QOpenGLDebugMessage& message) {
              qDebug() << message;
          });
          logger->startLogging(QOpenGLDebugLogger::LoggingMode::SynchronousLogging);
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

