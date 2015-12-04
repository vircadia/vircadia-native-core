//
//  Created by Bradley Austin Davis on 2015/05/21
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_GLWindow_h
#define hifi_GLWindow_h

#include <mutex>
#include <QtGui/QWindow>

class QOpenGLContext;
class QOpenGLDebugLogger;

class GLWindow : public QWindow {
public:
    virtual ~GLWindow();
    void createContext(QOpenGLContext* shareContext = nullptr);
    void createContext(const QSurfaceFormat& format, QOpenGLContext* shareContext = nullptr);
    bool makeCurrent();
    void doneCurrent();
    void swapBuffers();
    QOpenGLContext* context() const;
private:
    std::once_flag _reportOnce;
    QOpenGLContext* _context{ nullptr };
};

#endif
