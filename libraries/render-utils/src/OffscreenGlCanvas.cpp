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

OffscreenGlCanvas::OffscreenGlCanvas() {
}

void OffscreenGlCanvas::create(QOpenGLContext* sharedContext) {
    if (nullptr != sharedContext) {
        sharedContext->doneCurrent();
        _context.setFormat(sharedContext->format());
        _context.setShareContext(sharedContext);
    } else {
        QSurfaceFormat format;
        format.setDepthBufferSize(16);
        format.setStencilBufferSize(8);
        format.setMajorVersion(4);
        format.setMinorVersion(1);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
        _context.setFormat(format);
    }
    _context.create();

    _offscreenSurface.setFormat(_context.format());
    _offscreenSurface.create();
}

bool OffscreenGlCanvas::makeCurrent() {
    return _context.makeCurrent(&_offscreenSurface);
}

void OffscreenGlCanvas::doneCurrent() {
    _context.doneCurrent();
}

