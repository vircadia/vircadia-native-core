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
<<<<<<< HEAD
        _primaryFramebuffer.reset();
=======
        _primaryFramebufferFull.reset();
        _primaryFramebufferDepthColor.reset();
>>>>>>> f66492b4448218a241b19f0803ab5d6dde499113
        _primaryDepthTexture.reset();
        _primaryColorTexture.reset();
        _primaryNormalTexture.reset();
        _primarySpecularTexture.reset();
<<<<<<< HEAD
=======
        _selfieFramebuffer.reset();
>>>>>>> f66492b4448218a241b19f0803ab5d6dde499113
        _cachedFramebuffers.clear();
    }
}

void FramebufferCache::createPrimaryFramebuffer() {
<<<<<<< HEAD
    _primaryFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
=======
    _primaryFramebufferFull = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _primaryFramebufferDepthColor = gpu::FramebufferPointer(gpu::Framebuffer::create());
>>>>>>> f66492b4448218a241b19f0803ab5d6dde499113

    auto colorFormat = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
    auto width = _frameBufferSize.width();
    auto height = _frameBufferSize.height();

    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
    _primaryColorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _primaryNormalTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _primarySpecularTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));

<<<<<<< HEAD
    _primaryFramebuffer->setRenderBuffer(0, _primaryColorTexture);
    _primaryFramebuffer->setRenderBuffer(1, _primaryNormalTexture);
    _primaryFramebuffer->setRenderBuffer(2, _primarySpecularTexture);

=======
    _primaryFramebufferFull->setRenderBuffer(0, _primaryColorTexture);
    _primaryFramebufferFull->setRenderBuffer(1, _primaryNormalTexture);
    _primaryFramebufferFull->setRenderBuffer(2, _primarySpecularTexture);

    _primaryFramebufferDepthColor->setRenderBuffer(0, _primaryColorTexture);
>>>>>>> f66492b4448218a241b19f0803ab5d6dde499113

    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);
    _primaryDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, width, height, defaultSampler));

<<<<<<< HEAD
    _primaryFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);
}

gpu::FramebufferPointer FramebufferCache::getPrimaryFramebuffer() {
    if (!_primaryFramebuffer) {
        createPrimaryFramebuffer();
    }
    return _primaryFramebuffer;
}

=======
    _primaryFramebufferFull->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _primaryFramebufferDepthColor->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);
    
    _selfieFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    auto tex = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width * 0.5, height * 0.5, defaultSampler));
    _selfieFramebuffer->setRenderBuffer(0, tex);
}

gpu::FramebufferPointer FramebufferCache::getPrimaryFramebuffer() {
    if (!_primaryFramebufferFull) {
        createPrimaryFramebuffer();
    }
    return _primaryFramebufferFull;
}

gpu::FramebufferPointer FramebufferCache::getPrimaryFramebufferDepthColor() {
    if (!_primaryFramebufferDepthColor) {
        createPrimaryFramebuffer();
    }
    return _primaryFramebufferDepthColor;
}
>>>>>>> f66492b4448218a241b19f0803ab5d6dde499113


gpu::TexturePointer FramebufferCache::getPrimaryDepthTexture() {
    if (!_primaryDepthTexture) {
        createPrimaryFramebuffer();
    }
    return _primaryDepthTexture;
}

gpu::TexturePointer FramebufferCache::getPrimaryColorTexture() {
    if (!_primaryColorTexture) {
        createPrimaryFramebuffer();
    }
    return _primaryColorTexture;
}

gpu::TexturePointer FramebufferCache::getPrimaryNormalTexture() {
    if (!_primaryNormalTexture) {
        createPrimaryFramebuffer();
    }
    return _primaryNormalTexture;
}

gpu::TexturePointer FramebufferCache::getPrimarySpecularTexture() {
    if (!_primarySpecularTexture) {
        createPrimaryFramebuffer();
    }
    return _primarySpecularTexture;
}

<<<<<<< HEAD
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


=======
>>>>>>> f66492b4448218a241b19f0803ab5d6dde499113
gpu::FramebufferPointer FramebufferCache::getFramebuffer() {
    if (_cachedFramebuffers.isEmpty()) {
        _cachedFramebuffers.push_back(gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32, _frameBufferSize.width(), _frameBufferSize.height())));
    }
    gpu::FramebufferPointer result = _cachedFramebuffers.front();
    _cachedFramebuffers.pop_front();
    return result;
}

<<<<<<< HEAD

=======
>>>>>>> f66492b4448218a241b19f0803ab5d6dde499113
void FramebufferCache::releaseFramebuffer(const gpu::FramebufferPointer& framebuffer) {
    if (QSize(framebuffer->getSize().x, framebuffer->getSize().y) == _frameBufferSize) {
        _cachedFramebuffers.push_back(framebuffer);
    }
}

gpu::FramebufferPointer FramebufferCache::getShadowFramebuffer() {
    if (!_shadowFramebuffer) {
        const int SHADOW_MAP_SIZE = 2048;
        _shadowFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::createShadowmap(SHADOW_MAP_SIZE));
    }
    return _shadowFramebuffer;
}
<<<<<<< HEAD
=======

gpu::FramebufferPointer FramebufferCache::getSelfieFramebuffer() {
    if (!_selfieFramebuffer) {
        createPrimaryFramebuffer();
    }
    return _selfieFramebuffer;
}
>>>>>>> f66492b4448218a241b19f0803ab5d6dde499113
