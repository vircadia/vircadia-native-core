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

#include <stdint.h>

class QOpenGLContext;
class QSurface;
class QSurfaceFormat;
class QThread;

class QOpenGLContextWrapper {
public:
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