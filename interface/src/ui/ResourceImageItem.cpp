//
// ResourceImageItem.cpp
//
// Created by David Kelly and Howard Stearns on 2017/06/08
// Copyright 2017 High Fidelity, Inc.

// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "ResourceImageItem.h"

#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QOpenGLContext>

#include <DependencyManager.h>

ResourceImageItem::ResourceImageItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimer()));
}

void ResourceImageItem::onUpdateTimer() {
    update();
}

void ResourceImageItem::setUrl(const QString& url) {
    if (url != m_url) {
        m_url = url;
    }
    update();
}

void ResourceImageItem::setReady(bool ready) {
    if (ready != m_ready) {
        m_ready = ready;
        if (m_ready) {
            // 10 HZ for now.  Also this serves as a small
            // delay before getting the network texture, which
            // helps a lot.  TODO: find better way.
            m_updateTimer.start(100);
        } else {
            m_updateTimer.stop();
            update();
        }
    }
}

void ResourceImageItemRenderer::synchronize(QQuickFramebufferObject* item) {
    ResourceImageItem* resourceImageItem = static_cast<ResourceImageItem*>(item);

    resourceImageItem->setFlag(QQuickItem::ItemHasContents);
    bool urlChanged = false;
    if( _url != resourceImageItem->getUrl()) {
        _url = resourceImageItem->getUrl();
        urlChanged = true;
    }
    bool readyChanged = false;
    if (_ready != resourceImageItem->getReady()) {
        _ready = resourceImageItem->getReady();
        readyChanged = true;
    }

    _window = resourceImageItem->window();
    if (_ready && !_url.isNull() && !_url.isEmpty() && (urlChanged || readyChanged || !_networkTexture)) {
        _networkTexture = DependencyManager::get<TextureCache>()->getTexture(_url);
    }

    if (_ready && _networkTexture && _networkTexture->isLoaded()) {
        if(_fboMutex.tryLock()) {
            invalidateFramebufferObject();
            qApp->getActiveDisplayPlugin()->copyTextureToQuickFramebuffer(_networkTexture, _copyFbo, &_fenceSync);
            _fboMutex.unlock();
        } else {
            qDebug() << "couldn't get a lock, using last frame";
        }

    }
    glFlush();

}

QOpenGLFramebufferObject* ResourceImageItemRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    _copyFbo = new QOpenGLFramebufferObject(size, format);
    _copyFbo->bind();
    return new QOpenGLFramebufferObject(size, format);
}

/*
// this is helpful to look for problems without the confusion of the framebufer stuff
void ResourceImageItemRenderer::render() {
    qDebug() << "rendering";
    static int colorInt = 0;
    float red = (float)((colorInt++ % 50)/(float)50);
    glClearColor(red, 0.0f, 0.0f, 0.5f + red/2.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    update();
    //_window->resetOpenGLState();
    // we only want to call Renderer::update once per timer tick, and
    // so waiting on the fenceSync and resetting it does that
    auto f = QOpenGLContext::currentContext()->extraFunctions();
    if (_fenceSync) {
        f->glWaitSync(_fenceSync, 0, GL_TIMEOUT_IGNORED);
        f->glDeleteSync(_fenceSync);
        _fenceSync = 0;
        update();
    }
}
*/
void ResourceImageItemRenderer::render() {
    auto f = QOpenGLContext::currentContext()->extraFunctions();
    bool doUpdate = false;
    // black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (_fenceSync) {
        f->glWaitSync(_fenceSync, 0, GL_TIMEOUT_IGNORED);
        f->glDeleteSync(_fenceSync);
        _fenceSync = 0;
        doUpdate = true;
    }
    if (_ready) {
        _fboMutex.lock();
        _copyFbo->bind();
        QOpenGLFramebufferObject::blitFramebuffer(framebufferObject(), _copyFbo, GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        _copyFbo->release();
        _fboMutex.unlock();
        if (doUpdate) {
            update();
        }
    }
    glFlush();
}
