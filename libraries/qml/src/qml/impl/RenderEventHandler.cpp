//
//  Created by Bradley Austin Davis on 2018-01-04
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderEventHandler.h"

#ifndef DISABLE_QML

#include <gl/Config.h>
#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLHelpers.h>

#include <QtQuick/QQuickWindow>

#include <shared/NsightHelpers.h>
#include "Profiling.h"
#include "SharedObject.h"
#include "TextureCache.h"
#include "RenderControl.h"
#include "../Logging.h"

using namespace hifi::qml::impl;

bool RenderEventHandler::event(QEvent* e) {
    switch (static_cast<OffscreenEvent::Type>(e->type())) {
        case OffscreenEvent::Render:
            onRender();
            return true;

        case OffscreenEvent::Initialize:
            onInitalize();
            return true;

        case OffscreenEvent::Quit:
            onQuit();
            return true;

        default:
            break;
    }
    return QObject::event(e);
}

RenderEventHandler::RenderEventHandler(SharedObject* shared, QThread* targetThread)
    : _shared(shared) {
    // Create the GL canvas in the same thread as the share canvas
    if (!_canvas.create(SharedObject::getSharedContext())) {
        qFatal("Unable to create new offscreen GL context");
    }

    _canvas.moveToThreadWithContext(targetThread);
    moveToThread(targetThread);
}

void RenderEventHandler::onInitalize() {
    if (_shared->isQuit()) {
        return;
    }

    _canvas.setThreadContext();
    if (!_canvas.makeCurrent()) {
        qFatal("Unable to make QML rendering context current on render thread");
    }
    _shared->initializeRenderControl(_canvas.getContext());
}

void RenderEventHandler::resize() {
    PROFILE_RANGE(render_qml_gl, __FUNCTION__);
    auto targetSize = _shared->getSize();
    if (_currentSize != targetSize) {
        auto& offscreenTextures = SharedObject::getTextureCache();
        // Release hold on the textures of the old size
        if (_currentSize != QSize()) {
            _shared->releaseTextureAndFence();
            offscreenTextures.releaseSize(_currentSize);
        }

        _currentSize = targetSize;

        // Acquire the new texture size
        if (_currentSize != QSize()) {
            qCDebug(qmlLogging) << "Upating offscreen textures to " << _currentSize.width() << " x " << _currentSize.height();
            offscreenTextures.acquireSize(_currentSize);
            if (_depthStencil) {
                glDeleteRenderbuffers(1, &_depthStencil);
                _depthStencil = 0;
            }
            glGenRenderbuffers(1, &_depthStencil);
            glBindRenderbuffer(GL_RENDERBUFFER, _depthStencil);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _currentSize.width(), _currentSize.height());
            if (!_fbo) {
                glGenFramebuffers(1, &_fbo);
            }
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthStencil);
            glViewport(0, 0, _currentSize.width(), _currentSize.height());
            glScissor(0, 0, _currentSize.width(), _currentSize.height());
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }
    }
}

void RenderEventHandler::onRender() {
    if (_shared->isQuit()) {
        return;
    }

    if (_canvas.getContext() != QOpenGLContextWrapper::currentContext()) {
        qFatal("QML rendering context not current on render thread");
    }

    PROFILE_RANGE(render_qml_gl, __FUNCTION__);

    gl::globalLock();
    if (!_shared->preRender()) {
        return;
    }

    resize();

    {
        PROFILE_RANGE(render_qml_gl, "render");
        GLuint texture = SharedObject::getTextureCache().acquireTexture(_currentSize);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
        if (nsightActive()) {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
        } else {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            _shared->_quickWindow->setRenderTarget(_fbo, _currentSize);
            _shared->_renderControl->render();
        }
        _shared->_lastRenderTime = usecTimestampNow();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        // Fence will be used in another thread / context, so a flush is required
        glFlush();
        _shared->updateTextureAndFence({ texture, fence });
        _shared->_quickWindow->resetOpenGLState();
    }
    gl::globalRelease();
}

void RenderEventHandler::onQuit() {
    if (_canvas.getContext() != QOpenGLContextWrapper::currentContext()) {
        qFatal("QML rendering context not current on render thread");
    }

    if (_depthStencil) {
        glDeleteRenderbuffers(1, &_depthStencil);
        _depthStencil = 0;
    }

    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }

    _shared->shutdownRendering(_canvas, _currentSize);
    _canvas.doneCurrent();
    _canvas.moveToThreadWithContext(qApp->thread());
    moveToThread(qApp->thread());
    QThread::currentThread()->quit();
}

#endif
