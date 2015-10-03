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
        _primaryFramebufferFull.reset();
        _primaryFramebufferDepthColor.reset();
        _primaryFramebufferStencilColor.reset();
        _primaryDepthTexture.reset();
        _primaryStencilTexture.reset();
        _primaryColorTexture.reset();
        _primaryNormalTexture.reset();
        _primarySpecularTexture.reset();
        _selfieFramebuffer.reset();
        _cachedFramebuffers.clear();
    }
}

void FramebufferCache::createPrimaryFramebuffer() {
    _primaryFramebufferFull = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _primaryFramebufferDepthColor = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _primaryFramebufferStencilColor = gpu::FramebufferPointer(gpu::Framebuffer::create());

    auto colorFormat = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
    auto width = _frameBufferSize.width();
    auto height = _frameBufferSize.height();

    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
    _primaryColorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _primaryNormalTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _primarySpecularTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));

    _primaryFramebufferFull->setRenderBuffer(0, _primaryColorTexture);
    _primaryFramebufferFull->setRenderBuffer(1, _primaryNormalTexture);
    _primaryFramebufferFull->setRenderBuffer(2, _primarySpecularTexture);

    _primaryFramebufferDepthColor->setRenderBuffer(0, _primaryColorTexture);

    _primaryFramebufferStencilColor->setRenderBuffer(0, _primaryColorTexture);

    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);
    _primaryDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, width, height, defaultSampler));

    auto stencilFormat = gpu::Element(gpu::VEC2, gpu::UINT32, gpu::DEPTH_STENCIL);
    _primaryStencilTexture = gpu::TexturePointer(gpu::Texture::create2D(stencilFormat, width, height, defaultSampler));

    _primaryFramebufferFull->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _primaryFramebufferDepthColor->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _primaryFramebufferStencilColor->setDepthStencilBuffer(_primaryStencilTexture, stencilFormat);
    
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

gpu::FramebufferPointer FramebufferCache::getPrimaryFramebufferStencilColor() {
    if (!_primaryFramebufferStencilColor) {
        createPrimaryFramebuffer();
    }
    return _primaryFramebufferStencilColor;
}

gpu::TexturePointer FramebufferCache::getPrimaryDepthTexture() {
    if (!_primaryDepthTexture) {
        createPrimaryFramebuffer();
    }
    return _primaryDepthTexture;
}

gpu::TexturePointer FramebufferCache::getPrimaryStencilTexture() {
    if (!_primaryStencilTexture) {
        createPrimaryFramebuffer();
    }
    return _primaryStencilTexture;
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

gpu::FramebufferPointer FramebufferCache::getFramebuffer() {
    if (_cachedFramebuffers.isEmpty()) {
        _cachedFramebuffers.push_back(gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_RGBA_32, _frameBufferSize.width(), _frameBufferSize.height())));
    }
    gpu::FramebufferPointer result = _cachedFramebuffers.front();
    _cachedFramebuffers.pop_front();
    return result;
}

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

gpu::FramebufferPointer FramebufferCache::getSelfieFramebuffer() {
    if (!_selfieFramebuffer) {
        createPrimaryFramebuffer();
    }
    return _selfieFramebuffer;
}
