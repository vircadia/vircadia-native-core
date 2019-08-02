//
// ResourceImageItem.cpp
//
// Created by David Kelly and Howard Stearns on 2017/06/08
// Copyright 2017 High Fidelity, Inc.

// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ResourceImageItem.h"

#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLShaderProgram>

#include <plugins/DisplayPlugin.h>


static const char* VERTEX_SHADER = R"SHADER(
#version 450 core

out vec2 vTexCoord;

void main(void) {
    const float depth = 0.0;
    const vec4 UNIT_QUAD[4] = vec4[4](
        vec4(-1.0, -1.0, depth, 1.0),
        vec4(1.0, -1.0, depth, 1.0),
        vec4(-1.0, 1.0, depth, 1.0),
        vec4(1.0, 1.0, depth, 1.0)
    );
    vec4 pos = UNIT_QUAD[gl_VertexID];

    gl_Position = pos;
    vTexCoord = (pos.xy + 1.0) * 0.5;
}
)SHADER";

static const char* FRAGMENT_SHADER = R"SHADER(
#version 450 core

uniform sampler2D sampler;

in vec2 vTexCoord;

out vec4 FragColor;

vec3 color_LinearTosRGB(vec3 lrgb) {
    return mix(vec3(1.055) * pow(vec3(lrgb), vec3(0.41666)) - vec3(0.055), vec3(lrgb) * vec3(12.92), vec3(lessThan(lrgb, vec3(0.0031308))));
}

void main() {
    FragColor = vec4(color_LinearTosRGB(texture(sampler, vTexCoord).rgb), 1.0);
}
)SHADER";


ResourceImageItem::ResourceImageItem() : QQuickFramebufferObject() {
    auto textureCache = DependencyManager::get<TextureCache>();
    connect(textureCache.data(), SIGNAL(spectatorCameraFramebufferReset()), this, SLOT(update()));
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
        update();
    }
}

void ResourceImageItemRenderer::onUpdateTimer() {
    if (_ready) {
        if (_networkTexture && _networkTexture->isLoaded()) {
            if(_fboMutex.tryLock()) {
                invalidateFramebufferObject();
                qApp->getActiveDisplayPlugin()->copyTextureToQuickFramebuffer(_networkTexture, _copyFbo, &_fenceSync);
                _fboMutex.unlock();
            } else {
                qDebug() << "couldn't get a lock, using last frame";
            }
        } else {
            _networkTexture = DependencyManager::get<TextureCache>()->getTexture(_url);
        }
    }
    update();
}

ResourceImageItemRenderer::ResourceImageItemRenderer() : QQuickFramebufferObject::Renderer() {
    connect(&_updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimer()));
    auto textureCache = DependencyManager::get<TextureCache>();
}

void ResourceImageItemRenderer::synchronize(QQuickFramebufferObject* item) {
    ResourceImageItem* resourceImageItem = static_cast<ResourceImageItem*>(item);

    resourceImageItem->setFlag(QQuickItem::ItemHasContents);

    _url = resourceImageItem->getUrl();
    _ready = resourceImageItem->getReady();
    _visible = resourceImageItem->isVisible();
    _window = resourceImageItem->window();

    _networkTexture = DependencyManager::get<TextureCache>()->getTexture(_url);
    static const int UPDATE_TIMER_DELAY_IN_MS = 100; // 100 ms = 10 hz for now
    if (_ready && _visible && !_updateTimer.isActive()) {
        _updateTimer.start(UPDATE_TIMER_DELAY_IN_MS);
    } else if (!(_ready && _visible) && _updateTimer.isActive()) {
        _updateTimer.stop();
    }
}

QOpenGLFramebufferObject* ResourceImageItemRenderer::createFramebufferObject(const QSize& size) {
    if (_copyFbo) {
        delete _copyFbo;
    }
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    _copyFbo = new QOpenGLFramebufferObject(size, format);
    _copyFbo->bind();
    return new QOpenGLFramebufferObject(size, format);
}

void ResourceImageItemRenderer::render() {
    if (_fenceSync) {
        glWaitSync(_fenceSync, 0, GL_TIMEOUT_IGNORED);
        glDeleteSync(_fenceSync);
        _fenceSync = 0;
    }
    if (_ready) {
        _fboMutex.lock();


        if (!_shader) {
            _shader = new QOpenGLShaderProgram();
            _shader->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, VERTEX_SHADER);
            _shader->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, FRAGMENT_SHADER);
            _shader->link();
            glGenVertexArrays(1, &_vao);
        }
        framebufferObject()->bind();
        _shader->bind();

        auto sourceTextureId = _copyFbo->takeTexture();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sourceTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDeleteTextures(1, &sourceTextureId);

        _copyFbo->bind();
        _copyFbo->release();
        _fboMutex.unlock();
    }
    glFlush();
    _window->resetOpenGLState();
}
