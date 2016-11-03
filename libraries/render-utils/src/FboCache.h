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
#ifndef hifi_FboCache_h
#define hifi_FboCache_h

#include <QOffscreenSurface>
#include <QQueue>
#include <QMap>
#include <QSharedPointer>
#include <QMutex>

class QOpenGLFramebufferObject;

class FboCache : public QObject {
public:
    FboCache();

    // setSize() and getReadyFbo() must consitently be called from only a single 
    // thread.  Additionally, it is the caller's responsibility to ensure that 
    // the appropriate OpenGL context is active when doing so.

    // Important.... textures are sharable resources, but FBOs ARE NOT.  
    void setSize(const QSize& newSize);
    QOpenGLFramebufferObject* getReadyFbo();

    // These operations are thread safe and require no OpenGL context.  They manipulate the 
    // internal locks and  pointers but execute no OpenGL opreations.
    void lockTexture(int texture);
    void releaseTexture(int texture);
    
    const QSize& getSize();

protected:
    QMap<int, QSharedPointer<QOpenGLFramebufferObject>> _fboMap;
    QMap<int, int> _fboLocks;
    QQueue<QOpenGLFramebufferObject*> _readyFboQueue;
    QQueue<QSharedPointer<QOpenGLFramebufferObject>> _destroyFboQueue;
    QMutex _lock;
    QSize _size;

};

#endif // hifi_FboCache_h
