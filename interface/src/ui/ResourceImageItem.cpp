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
#include <DependencyManager.h>
#include <TextureCache.h>


ResourceImageItem::ResourceImageItem(QQuickItem* parent) : QQuickFramebufferObject(parent) {
    connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(update()));
}

void ResourceImageItem::setUrl(const QString& url) {
    if (url != m_url) {
        m_url = url;
        update();
    }
}

void ResourceImageItem::setReady(bool ready) {
    if (ready != m_ready) {
        m_ready = ready;
        if (m_ready) {
            m_updateTimer.start(100);
        } else {
            m_updateTimer.stop();
        }
        update();
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

    if (!_ready && readyChanged) {
        qDebug() << "clearing network texture!!!!!!!!!!!!!!!!!";
        _networkTexture.clear();
    }

    _window = resourceImageItem->window();
    if (_ready && !_url.isNull() && !_url.isEmpty() && (urlChanged || readyChanged || !_networkTexture)) {
        _networkTexture = DependencyManager::get<TextureCache>()->getTexture(_url);
    }

    if (_networkTexture) {
        qDebug() << "copying texture";
        auto texture = _networkTexture->getGPUTexture();
        if (texture) {
            if (_fboMutex.tryLock()) {
                qApp->getActiveDisplayPlugin()->copyTextureToQuickFramebuffer(texture, framebufferObject());
                _fboMutex.unlock();
            }
        }
    }
}

QOpenGLFramebufferObject* ResourceImageItemRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
}

void ResourceImageItemRenderer::render() {
    _fboMutex.lock();
    _window->resetOpenGLState();
    _fboMutex.unlock();
}
