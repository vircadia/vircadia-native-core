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
        _primaryFramebuffer.reset();
        _primaryDepthTexture.reset();
        _primaryColorTexture.reset();
        _deferredFramebuffer.reset();
        _deferredFramebufferDepthColor.reset();
        _deferredColorTexture.reset();
        _deferredNormalTexture.reset();
        _deferredSpecularTexture.reset();
        _selfieFramebuffer.reset();
        _cachedFramebuffers.clear();
        _lightingTexture.reset();
        _lightingFramebuffer.reset();
    }
}

void FramebufferCache::createPrimaryFramebuffer() {
    _primaryFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _deferredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _deferredFramebufferDepthColor = gpu::FramebufferPointer(gpu::Framebuffer::create());

    auto colorFormat = gpu::Element::COLOR_RGBA_32;
    auto width = _frameBufferSize.width();
    auto height = _frameBufferSize.height();

    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
    _primaryColorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));

    _primaryFramebuffer->setRenderBuffer(0, _primaryColorTexture);

    _deferredColorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _deferredNormalTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _deferredSpecularTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));

    _deferredFramebuffer->setRenderBuffer(0, _deferredColorTexture);
    _deferredFramebuffer->setRenderBuffer(1, _deferredNormalTexture);
    _deferredFramebuffer->setRenderBuffer(2, _deferredSpecularTexture);

    _deferredFramebufferDepthColor->setRenderBuffer(0, _deferredColorTexture);

  //  auto depthFormat = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);
    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format
    _primaryDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, width, height, defaultSampler));

    _primaryFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _deferredFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _deferredFramebufferDepthColor->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);


    _selfieFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    auto tex = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width * 0.5, height * 0.5, defaultSampler));
    _selfieFramebuffer->setRenderBuffer(0, tex);

    auto smoothSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR);

    // FIXME: Decide on the proper one, let s stick to R11G11B10 for now
    //_lightingTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element::COLOR_RGBA_32, width, height, defaultSampler));
    _lightingTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC3, gpu::NUINT8, gpu::R11G11B10), width, height, defaultSampler));
    //_lightingTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::VEC4, gpu::HALF, gpu::RGBA), width, height, defaultSampler));
    _lightingFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _lightingFramebuffer->setRenderBuffer(0, _lightingTexture);
    _lightingFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);
}

gpu::FramebufferPointer FramebufferCache::getPrimaryFramebuffer() {
    if (!_primaryFramebuffer) {
        createPrimaryFramebuffer();
    }
    return _primaryFramebuffer;
}

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

gpu::FramebufferPointer FramebufferCache::getDeferredFramebuffer() {
    if (!_deferredFramebuffer) {
        createPrimaryFramebuffer();
    }
    return _deferredFramebuffer;
}

gpu::FramebufferPointer FramebufferCache::getDeferredFramebufferDepthColor() {
    if (!_deferredFramebufferDepthColor) {
        createPrimaryFramebuffer();
    }
    return _deferredFramebufferDepthColor;
}

gpu::TexturePointer FramebufferCache::getDeferredColorTexture() {
    if (!_deferredColorTexture) {
        createPrimaryFramebuffer();
    }
    return _deferredColorTexture;
}

gpu::TexturePointer FramebufferCache::getDeferredNormalTexture() {
    if (!_deferredNormalTexture) {
        createPrimaryFramebuffer();
    }
    return _deferredNormalTexture;
}

gpu::TexturePointer FramebufferCache::getDeferredSpecularTexture() {
    if (!_deferredSpecularTexture) {
        createPrimaryFramebuffer();
    }
    return _deferredSpecularTexture;
}

gpu::FramebufferPointer FramebufferCache::getLightingFramebuffer() {
    if (!_lightingFramebuffer) {
        createPrimaryFramebuffer();
    }
    return _lightingFramebuffer;
}

gpu::TexturePointer FramebufferCache::getLightingTexture() {
    if (!_lightingTexture) {
        createPrimaryFramebuffer();
    }
    return _lightingTexture;
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

gpu::FramebufferPointer FramebufferCache::getSelfieFramebuffer() {
    if (!_selfieFramebuffer) {
        createPrimaryFramebuffer();
    }
    return _selfieFramebuffer;
}
