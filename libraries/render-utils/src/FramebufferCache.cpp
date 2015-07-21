//
//  FramebufferCache.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 8/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FramebufferCache.h"

#include <mutex>

#include <glm/glm.hpp>

#include <QMap>
#include <QQueue>
#include <gpu/Batch.h>
#include <gpu/GPUConfig.h>
#include "RenderUtilsLogging.h"

static QQueue<gpu::FramebufferPointer> _cachedFramebuffers;

FramebufferCache::FramebufferCache() {
}

FramebufferCache::~FramebufferCache() {
    _cachedFramebuffers.clear();
}

void FramebufferCache::setFrameBufferSize(QSize frameBufferSize) {
    //If the size changed, we need to delete our FBOs
    if (_frameBufferSize != frameBufferSize) {
        _frameBufferSize = frameBufferSize;
        _primaryFramebuffer.reset();
    }
}

void FramebufferCache::createPrimaryFramebuffer() {
    _primaryFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());

    static auto colorFormat = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
    static auto depthFormat = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);

    auto width = _frameBufferSize.width();
    auto height = _frameBufferSize.height();

    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
    auto colorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    auto normalTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    auto specularTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));

    _primaryFramebuffer->setRenderBuffer(0, colorTexture);
    _primaryFramebuffer->setRenderBuffer(1, normalTexture);
    _primaryFramebuffer->setRenderBuffer(2, specularTexture);


    auto depthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, width, height, defaultSampler));
    _primaryFramebuffer->setDepthStencilBuffer(depthTexture, depthFormat);
}

gpu::FramebufferPointer FramebufferCache::getPrimaryFramebuffer() {
    if (!_primaryFramebuffer) {
        createPrimaryFramebuffer();
    }
    return _primaryFramebuffer;
}

void FramebufferCache::setPrimaryDrawBuffers(gpu::Batch& batch, bool color, bool normal, bool specular) {
    GLenum buffers[3];
    int bufferCount = 0;
    if (color) {
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT0;
    }
    if (normal) {
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT1;
    }
    if (specular) {
        buffers[bufferCount++] = GL_COLOR_ATTACHMENT2;
    }
    batch._glDrawBuffers(bufferCount, buffers);
}


gpu::FramebufferPointer FramebufferCache::getFramebuffer() {
    if (_cachedFramebuffers.isEmpty()) {
        _cachedFramebuffers.push_back(gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32, _frameBufferSize.width(), _frameBufferSize.height())));
    }
    gpu::FramebufferPointer result = _cachedFramebuffers.front();
    _cachedFramebuffers.pop_front();
    return result;
}


void FramebufferCache::releaseFramebuffer(const gpu::FramebufferPointer& framebuffer) {
    _cachedFramebuffers.push_back(framebuffer);
}

gpu::FramebufferPointer FramebufferCache::getSecondaryFramebuffer() {
    static auto _secondaryFramebuffer = getFramebuffer();
    return _secondaryFramebuffer;
}

gpu::FramebufferPointer FramebufferCache::getShadowFramebuffer() {
    if (!_shadowFramebuffer) {
        const int SHADOW_MAP_SIZE = 2048;
        _shadowFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::createShadowmap(SHADOW_MAP_SIZE));
    }
    return _shadowFramebuffer;
}
