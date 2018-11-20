//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderThread.h"

#include <mutex>

#include <jni.h>
#include <android/log.h>

#include <QtGui/QWindow>

#include <gl/GLHelpers.h>
#include <GLMHelpers.h>

#define MARGIN 48

void RenderThread::initialize(QWindow* window) {
    std::unique_lock<std::mutex> lock(_frameLock);
    setObjectName("RenderThread");
    Parent::initialize();

    _thread->setObjectName("RenderThread");
    _size = window->size();

    _glContext.setWindow(window);
    _glContext.create();
    _glContext.makeCurrent();

    gl::setSwapInterval(0);
    glGenFramebuffers(1, &_readFbo);
    OffscreenSurface::setSharedContext(_glContext.qglContext());

    // GPU library init
    _glContext.doneCurrent();
    _glContext.moveToThread(_thread);

    _offscreen = std::make_shared<OffscreenSurface>();
    _offscreen->resize({ _size.width() - (MARGIN * 2), _size.height() - (MARGIN * 2)});
    _offscreen->load(QUrl("qrc://qml/main.qml"));
}

void RenderThread::releaseTexture() {
    if (_uiTexture != 0) {
        auto readFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glFlush();
        CHECK_GL_ERROR();
        _discardLambda(_uiTexture, readFence);
        _uiTexture = 0;
    }
}

void RenderThread::setup() {
    // Wait until the context has been moved to this thread
    { std::unique_lock<std::mutex> lock(_frameLock); }
    _glContext.makeCurrent();
}

void RenderThread::shutdown() {
    releaseTexture();
    _glContext.doneCurrent();
}

bool RenderThread::process() {
    float now = secTimestampNow();
    float red = fabsf(sinf(now * PI));
    _glContext.makeCurrent();
    {
        OffscreenSurface::TextureAndFence newTextureAndFence;
        if (_offscreen->fetchTexture(newTextureAndFence)) {
            releaseTexture();
            const auto& newTexture = newTextureAndFence.first;
            GLsync writeFence = static_cast<GLsync>(newTextureAndFence.second);
            _uiTexture = newTexture;
            glWaitSync(writeFence, 0, GL_TIMEOUT_IGNORED);
            glDeleteSync(writeFence);
        }
    }

    glClearColor(1, red, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (_uiTexture != 0) {
        auto uiSize = _offscreen->size();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _readFbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _uiTexture, 0);
        glBlitFramebuffer(
            0, 0, uiSize.width(), uiSize.height(),
            MARGIN, MARGIN, uiSize.width()  + MARGIN, uiSize.height() + MARGIN,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    _glContext.swapBuffers();
    return true;
}
