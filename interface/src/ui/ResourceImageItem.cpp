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
            m_updateTimer.start(1000);
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
    _window = resourceImageItem->window();
    qDebug() << "synchronize called!!!!!!!";
    if (_ready && !_url.isNull() && !_url.isEmpty() && (readyChanged || urlChanged)) {
        _networkTexture = DependencyManager::get<TextureCache>()->getTexture(_url);
    }
}

QOpenGLFramebufferObject* ResourceImageItemRenderer::createFramebufferObject(const QSize& size) {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    return new QOpenGLFramebufferObject(size, format);
}

void ResourceImageItemRenderer::render() {
    qDebug() << "render called!!!!!!!!!!!!!!";
    if (_networkTexture && _ready) {
        auto texture = _networkTexture->getGPUTexture();
        if (texture) {
            qApp->getActiveDisplayPlugin()->copyTextureToQuickFramebuffer(texture, framebufferObject());
            _window->resetOpenGLState();
        }
    }
}
