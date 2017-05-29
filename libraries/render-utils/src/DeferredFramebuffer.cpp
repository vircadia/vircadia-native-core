//
//  DeferredFramebuffer.h
//  libraries/render-utils/src/
//
//  Created by Sam Gateau 7/11/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "DeferredFramebuffer.h"


DeferredFramebuffer::DeferredFramebuffer() {
}


void DeferredFramebuffer::updatePrimaryDepth(const gpu::TexturePointer& depthBuffer) {
    //If the depth buffer or size changed, we need to delete our FBOs
    bool reset = false;
    if ((_primaryDepthTexture != depthBuffer)) {
        _primaryDepthTexture = depthBuffer;
        reset = true;
    }
    if (_primaryDepthTexture) {
        auto newFrameSize = glm::ivec2(_primaryDepthTexture->getDimensions());
        if (_frameSize != newFrameSize) {
            _frameSize = newFrameSize;
            reset = true;
        }
    }

    if (reset) {
        _deferredFramebuffer.reset();
        _deferredFramebufferDepthColor.reset();
        _deferredColorTexture.reset();
        _deferredNormalTexture.reset();
        _deferredSpecularTexture.reset();
        _lightingTexture.reset();
        _lightingFramebuffer.reset();
    }
}

void DeferredFramebuffer::allocate() {

    _deferredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("deferred"));
    _deferredFramebufferDepthColor = gpu::FramebufferPointer(gpu::Framebuffer::create("deferredDepthColor"));

    auto colorFormat = gpu::Element::COLOR_SRGBA_32;
    auto linearFormat = gpu::Element::COLOR_RGBA_32;
    auto width = _frameSize.x;
    auto height = _frameSize.y;

    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);

    _deferredColorTexture = gpu::Texture::createRenderBuffer(colorFormat, width, height, gpu::Texture::SINGLE_MIP, defaultSampler);
    _deferredNormalTexture = gpu::Texture::createRenderBuffer(linearFormat, width, height, gpu::Texture::SINGLE_MIP, defaultSampler);
    _deferredSpecularTexture = gpu::Texture::createRenderBuffer(linearFormat, width, height, gpu::Texture::SINGLE_MIP, defaultSampler);

    _deferredFramebuffer->setRenderBuffer(0, _deferredColorTexture);
    _deferredFramebuffer->setRenderBuffer(1, _deferredNormalTexture);
    _deferredFramebuffer->setRenderBuffer(2, _deferredSpecularTexture);

    _deferredFramebufferDepthColor->setRenderBuffer(0, _deferredColorTexture);

    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format
    if (!_primaryDepthTexture) {
        _primaryDepthTexture = gpu::Texture::createRenderBuffer(depthFormat, width, height, gpu::Texture::SINGLE_MIP, defaultSampler);
    }

    _deferredFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _deferredFramebufferDepthColor->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);


    auto smoothSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR);

    _lightingTexture = gpu::Texture::createRenderBuffer(gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::R11G11B10), width, height, gpu::Texture::SINGLE_MIP, defaultSampler);
    _lightingFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("lighting"));
    _lightingFramebuffer->setRenderBuffer(0, _lightingTexture);
    _lightingFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _deferredFramebuffer->setRenderBuffer(3, _lightingTexture);

}


gpu::TexturePointer DeferredFramebuffer::getPrimaryDepthTexture() {
    if (!_primaryDepthTexture) {
        allocate();
    }
    return _primaryDepthTexture;
}

gpu::FramebufferPointer DeferredFramebuffer::getDeferredFramebuffer() {
    if (!_deferredFramebuffer) {
        allocate();
    }
    return _deferredFramebuffer;
}

gpu::FramebufferPointer DeferredFramebuffer::getDeferredFramebufferDepthColor() {
    if (!_deferredFramebufferDepthColor) {
        allocate();
    }
    return _deferredFramebufferDepthColor;
}

gpu::TexturePointer DeferredFramebuffer::getDeferredColorTexture() {
    if (!_deferredColorTexture) {
        allocate();
    }
    return _deferredColorTexture;
}

gpu::TexturePointer DeferredFramebuffer::getDeferredNormalTexture() {
    if (!_deferredNormalTexture) {
        allocate();
    }
    return _deferredNormalTexture;
}

gpu::TexturePointer DeferredFramebuffer::getDeferredSpecularTexture() {
    if (!_deferredSpecularTexture) {
        allocate();
    }
    return _deferredSpecularTexture;
}

gpu::FramebufferPointer DeferredFramebuffer::getLightingFramebuffer() {
    if (!_lightingFramebuffer) {
        allocate();
    }
    return _lightingFramebuffer;
}

gpu::TexturePointer DeferredFramebuffer::getLightingTexture() {
    if (!_lightingTexture) {
        allocate();
    }
    return _lightingTexture;
}
