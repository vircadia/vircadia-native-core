//
//  QOpenGLContextWrapper.h
//
//
//  Created by Clement on 12/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QOpenGLContextWrapper_h
#define hifi_QOpenGLContextWrapper_h

#include <QtGlobal>
#include <memory>

class QOpenGLContext;
class QSurface;
class QSurfaceFormat;
class QThread;

#if defined(Q_OS_ANDROID)
#include <EGL/egl.h>
#include <QtPlatformHeaders/QEGLNativeContext>
using QGLNativeContext = QEGLNativeContext;
#elif defined(Q_OS_WIN)
class QWGLNativeContext;
using QGLNativeContext = QWGLNativeContext;
#else
using QGLNativeContext = void*;
#endif

class QOpenGLContextWrapper {
public:
    using Pointer = std::shared_ptr<QOpenGLContextWrapper>;
    using NativeContextPointer = std::shared_ptr<QGLNativeContext>;
    static Pointer currentContextWrapper();


    QOpenGLContextWrapper();
    QOpenGLContextWrapper(QOpenGLContext* context);
    virtual ~QOpenGLContextWrapper();
    void setFormat(const QSurfaceFormat& format);
    bool create();
    void swapBuffers(QSurface* surface);
    bool makeCurrent(QSurface* surface);
    void doneCurrent();
    void setShareContext(QOpenGLContext* otherContext);
    void moveToThread(QThread* thread);

    NativeContextPointer getNativeContext() const;

    static QOpenGLContext* currentContext();
    static uint32_t currentContextVersion();

    QOpenGLContext* getContext() {
        return _context;
    }

    
private:
    bool _ownContext { false };
    QOpenGLContext* _context { nullptr };
};

bool isCurrentContext(QOpenGLContext* context);

#endif // hifi_QOpenGLContextWrapper_h