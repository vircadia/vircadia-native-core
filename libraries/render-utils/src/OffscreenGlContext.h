//
//  OffscreenGlCanvas.h
//  interface/src/renderer
//
//  Created by Bradley Austin Davis on 2014/04/09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_OffscreenGlContext_h
#define hifi_OffscreenGlContext_h

#include <QOpenGLContext>
#include <QOffscreenSurface>

class OffscreenGlContext : public QObject {
public:
    OffscreenGlContext();
    void create(QOpenGLContext * sharedContext = nullptr);
    bool makeCurrent();
    void doneCurrent();
    QOpenGLContext * getContext() {
        return &_context;
    }

protected:
    QOpenGLContext _context;
    QOffscreenSurface _offscreenSurface;
};

#endif // hifi_OffscreenGlCanvas_h
