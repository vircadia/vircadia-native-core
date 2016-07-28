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

#include <glm/glm.hpp>
#include <gpu/Format.h>
#include <gpu/Framebuffer.h>

#include "RenderUtilsLogging.h"

void FramebufferCache::setFrameBufferSize(QSize frameBufferSize) {
    //If the size changed, we need to delete our FBOs
    if (_frameBufferSize != frameBufferSize) {
        _frameBufferSize = frameBufferSize;
        _selfieFramebuffer.reset();
        _occlusionFramebuffer.reset();
        _occlusionTexture.reset();
        _occlusionBlurredFramebuffer.reset();
        _occlusionBlurredTexture.reset();
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cachedFramebuffers.clear();
        }
    }
}

void FramebufferCache::createPrimaryFramebuffer() {
    auto colorFormat = gpu::Element::COLOR_SRGBA_32;
    auto width = _frameBufferSize.width();
    auto height = _frameBufferSize.height();

    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);

    _selfieFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    auto tex = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width * 0.5, height * 0.5, defaultSampler));
    _selfieFramebuffer->setRenderBuffer(0, tex);

    auto smoothSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR);

    resizeAmbientOcclusionBuffers();
}


void FramebufferCache::resizeAmbientOcclusionBuffers() {
    _occlusionFramebuffer.reset();
    _occlusionTexture.reset();
    _occlusionBlurredFramebuffer.reset();
    _occlusionBlurredTexture.reset();


    auto width = _frameBufferSize.width() >> _AOResolutionLevel;
    auto height = _frameBufferSize.height() >> _AOResolutionLevel;
    auto colorFormat = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGB);
    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR);
    // auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format

    _occlusionTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _occlusionFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _occlusionFramebuffer->setRenderBuffer(0, _occlusionTexture);
 //   _occlusionFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _occlusionBlurredTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _occlusionBlurredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _occlusionBlurredFramebuffer->setRenderBuffer(0, _occlusionBlurredTexture);
   // _occlusionBlurredFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);
}


gpu::FramebufferPointer FramebufferCache::getFramebuffer() {
    std::unique_lock<std::mutex> lock(_mutex);
    if (_cachedFramebuffers.empty()) {
        _cachedFramebuffers.push_back(gpu::FramebufferPointer(gpu::Framebuffer::create(gpu::Element::COLOR_SRGBA_32, _frameBufferSize.width(), _frameBufferSize.height())));
    }
    gpu::FramebufferPointer result = _cachedFramebuffers.front();
    _cachedFramebuffers.pop_front();
    return result;
}

void FramebufferCache::releaseFramebuffer(const gpu::FramebufferPointer& framebuffer) {
    std::unique_lock<std::mutex> lock(_mutex);
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

void FramebufferCache::setAmbientOcclusionResolutionLevel(int level) {
    const int MAX_AO_RESOLUTION_LEVEL = 4;
    level = std::max(0, std::min(level, MAX_AO_RESOLUTION_LEVEL));
    if (level != _AOResolutionLevel) {
        _AOResolutionLevel = level;
        resizeAmbientOcclusionBuffers();
    }
}

gpu::FramebufferPointer FramebufferCache::getOcclusionFramebuffer() {
    if (!_occlusionFramebuffer) {
        resizeAmbientOcclusionBuffers();
    }
    return _occlusionFramebuffer;
}

gpu::TexturePointer FramebufferCache::getOcclusionTexture() {
    if (!_occlusionTexture) {
        resizeAmbientOcclusionBuffers();
    }
    return _occlusionTexture;
}

gpu::FramebufferPointer FramebufferCache::getOcclusionBlurredFramebuffer() {
    if (!_occlusionBlurredFramebuffer) {
        resizeAmbientOcclusionBuffers();
    }
    return _occlusionBlurredFramebuffer;
}

gpu::TexturePointer FramebufferCache::getOcclusionBlurredTexture() {
    if (!_occlusionBlurredTexture) {
        resizeAmbientOcclusionBuffers();
    }
    return _occlusionBlurredTexture;
}