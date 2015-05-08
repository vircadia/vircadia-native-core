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


#include "OffscreenGlContext.h"

OffscreenGlContext::OffscreenGlContext() {
}

void OffscreenGlContext::create(QOpenGLContext * sharedContext) {
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setMajorVersion(4);
    format.setMinorVersion(1);
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);

    _context.setFormat(format);
    if (nullptr != sharedContext) {
        _context.setShareContext(sharedContext);
    }
    _context.create();

    _offscreenSurface.setFormat(_context.format());
    _offscreenSurface.create();
}

bool OffscreenGlContext::makeCurrent() {
    return _context.makeCurrent(&_offscreenSurface);
}

void OffscreenGlContext::doneCurrent() {
    _context.doneCurrent();
}

