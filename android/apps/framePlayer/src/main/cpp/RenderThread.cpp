//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderThread.h"

#include <QtGui/QWindow>

void RenderThread::submitFrame(const gpu::FramePointer& frame) {
    std::unique_lock<std::mutex> lock(_frameLock);
    _pendingFrames.push(frame);
}

void RenderThread::move(const glm::vec3& v) {
    std::unique_lock<std::mutex> lock(_frameLock);
    _correction = glm::inverse(glm::translate(mat4(), v)) * _correction;
}

void RenderThread::setup() {
    // Wait until the context has been moved to this thread
    { std::unique_lock<std::mutex> lock(_frameLock); }

    makeCurrent();
    // Disable vsync for profiling
    ::gl::setSwapInterval(0);

    glClearColor(1, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    _glContext.swapBuffers();

    // GPU library init
    gpu::Context::init<gpu::gl::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();
    _backend = _gpuContext->getBackend();
    _gpuContext->beginFrame();
    _gpuContext->endFrame();
    makeCurrent();


    glGenFramebuffers(1, &_uiFbo);
    glGenTextures(1, &_externalTexture);
    glBindTexture(GL_TEXTURE_2D, _externalTexture);
    static const glm::u8vec4 color{ 0 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &color);

    glClearColor(0, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    _glContext.swapBuffers();
}

void RenderThread::initialize(QWindow* window, hifi::qml::OffscreenSurface* offscreen) {
    std::unique_lock<std::mutex> lock(_frameLock);
    setObjectName("RenderThread");
    Parent::initialize();

    _offscreen = offscreen;
    _window = window;
    _glContext.setWindow(_window);
    _glContext.create();
    _glContext.makeCurrent();

    hifi::qml::OffscreenSurface::setSharedContext(_glContext.qglContext());
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    _glContext.swapBuffers();
    _glContext.doneCurrent();
    _glContext.moveToThread(_thread);
    _thread->setObjectName("RenderThread");
}

void RenderThread::shutdown() {
    _activeFrame.reset();
    while (!_pendingFrames.empty()) {
        _gpuContext->consumeFrameUpdates(_pendingFrames.front());
        _pendingFrames.pop();
    }
    _gpuContext->shutdown();
    _gpuContext.reset();
}

void RenderThread::renderFrame() {
    auto windowSize = _window->geometry().size();
    uvec2 readFboSize;
    uint32_t readFbo{ 0 };

    if (_activeFrame) {
        const auto &frame = _activeFrame;
        _backend->recycle();
        _backend->syncCache();
        _gpuContext->enableStereo(frame->stereoState._enable);
        if (frame && !frame->batches.empty()) {
            _gpuContext->executeFrame(frame);
        }
        auto &glBackend = static_cast<gpu::gl::GLBackend&>(*_backend);
        readFbo = glBackend.getFramebufferID(frame->framebuffer);
        readFboSize = frame->framebuffer->getSize();
        CHECK_GL_ERROR();
    } else {
        hifi::qml::OffscreenSurface::TextureAndFence newTextureAndFence;
        if (_offscreen->fetchTexture(newTextureAndFence)) {
            if (_uiTexture != 0) {
                auto readFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                glFlush();
                _offscreen->getDiscardLambda()(_uiTexture, readFence);
                _uiTexture = 0;
            }

            glWaitSync((GLsync)newTextureAndFence.second, 0, GL_TIMEOUT_IGNORED);
            glDeleteSync((GLsync)newTextureAndFence.second);
            _uiTexture = newTextureAndFence.first;
            glBindFramebuffer(GL_READ_FRAMEBUFFER, _uiFbo);
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _uiTexture, 0);
        }

        if (_uiTexture != 0) {
            readFbo = _uiFbo;
            readFboSize = { windowSize.width(), windowSize.height() };
        }
    }

    if (readFbo) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
        glBlitFramebuffer(
            0, 0, readFboSize.x, readFboSize.y,
            0, 0, windowSize.width(), windowSize.height(),
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    } else {
        glClearColor(1, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    _glContext.swapBuffers();
}

void RenderThread::updateFrame() {
    std::queue<gpu::FramePointer> pendingFrames;
    {
        std::unique_lock<std::mutex> lock(_frameLock);
        pendingFrames.swap(_pendingFrames);
    }

    while (!pendingFrames.empty()) {
        _activeFrame = pendingFrames.front();
        pendingFrames.pop();
        if (_activeFrame) {
            _gpuContext->consumeFrameUpdates(_activeFrame);
        }
    }
}

bool RenderThread::process() {
    updateFrame();
    makeCurrent();
    renderFrame();
    return true;
}
