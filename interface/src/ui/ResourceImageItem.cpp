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
#include <TextureCache.h>

ResourceImageItem::ResourceImageItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(update()));
}

void ResourceImageItem::setUrl(const QString& url) {
    if (url != m_url) {
        m_url = url;
    }
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
    _window->setClearBeforeRendering(true);
    if (_ready && !_url.isNull() && !_url.isEmpty() && (urlChanged || readyChanged || !_networkTexture)) {
        _networkTexture = DependencyManager::get<TextureCache>()->getTexture(_url);
    }

    if (_ready && _networkTexture && _networkTexture->isLoaded()) {
        if(_fboMutex.tryLock()) {
            qApp->getActiveDisplayPlugin()->copyTextureToQuickFramebuffer(_networkTexture, _copyFbo, &_fenceSync);
            _fboMutex.unlock();
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

void ResourceImageItemRenderer::render() {
    qDebug() << "initial error" << glGetError();
    /*glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);*/
    QOpenGLExtraFunctions* f = QOpenGLContext::currentContext()->extraFunctions();
    _fboMutex.lock();
    if (_fenceSync) {
        f->glWaitSync(_fenceSync, 0, GL_TIMEOUT_IGNORED);
        qDebug() << "wait error" << f->glGetError();
    }
    if (_ready) {
        f->glBindTexture(GL_TEXTURE_2D, _copyFbo->texture());
        qDebug() << "bind tex error" << f->glGetError() << "texture" << _copyFbo->texture();
        GLint internalFormat { 0 };
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
        qDebug() << "getTexLevelParameteriv error" << f->glGetError() << internalFormat;
        f->glCopyTexImage2D(GL_TEXTURE_2D, 0, (GLenum)internalFormat, 0, 0, _copyFbo->width(), _copyFbo->height(), 0);
        qDebug() << "copy error" << f->glGetError();
    }
    _fboMutex.unlock();
}
