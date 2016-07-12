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


void DeferredFramebuffer::setPrimaryDepth(const gpu::TexturePointer& depthBuffer) {
    //If the size changed, we need to delete our FBOs
    if (_primaryDepthTexture != depthBuffer) {
        _primaryDepthTexture = depthBuffer;

    }
}

void DeferredFramebuffer::setFrameSize(const glm::ivec2& frameBufferSize) {
    //If the size changed, we need to delete our FBOs
    if (_frameSize != frameBufferSize) {
        _frameSize = frameBufferSize;
         _deferredFramebuffer.reset();
        _deferredFramebufferDepthColor.reset();
        _deferredColorTexture.reset();
        _deferredNormalTexture.reset();
        _deferredSpecularTexture.reset();
        _lightingTexture.reset();
        _lightingFramebuffer.reset();

        _occlusionFramebuffer.reset();
        _occlusionTexture.reset();
        _occlusionBlurredFramebuffer.reset();
        _occlusionBlurredTexture.reset();
    }
}

void DeferredFramebuffer::allocate() {
    _deferredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _deferredFramebufferDepthColor = gpu::FramebufferPointer(gpu::Framebuffer::create());

    auto colorFormat = gpu::Element::COLOR_SRGBA_32;
    auto linearFormat = gpu::Element::COLOR_RGBA_32;
    auto width = _frameSize.x;
    auto height = _frameSize.y;

    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_POINT);
   // _primaryColorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));

  //  _primaryFramebuffer->setRenderBuffer(0, _primaryColorTexture);

    _deferredColorTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));

    _deferredNormalTexture = gpu::TexturePointer(gpu::Texture::create2D(linearFormat, width, height, defaultSampler));
    _deferredSpecularTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));

    _deferredFramebuffer->setRenderBuffer(0, _deferredColorTexture);
    _deferredFramebuffer->setRenderBuffer(1, _deferredNormalTexture);
    _deferredFramebuffer->setRenderBuffer(2, _deferredSpecularTexture);

    _deferredFramebufferDepthColor->setRenderBuffer(0, _deferredColorTexture);

    //  auto depthFormat = gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::DEPTH);

    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format
    if (!_primaryDepthTexture) {
        _primaryDepthTexture = gpu::TexturePointer(gpu::Texture::create2D(depthFormat, width, height, defaultSampler));
    }

    _deferredFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _deferredFramebufferDepthColor->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);


    auto smoothSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR);

    _lightingTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::R11G11B10), width, height, defaultSampler));
    _lightingFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _lightingFramebuffer->setRenderBuffer(0, _lightingTexture);
    _lightingFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _deferredFramebuffer->setRenderBuffer(3, _lightingTexture);

    // For AO:
   // auto pointMipSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_POINT);
 //   _depthPyramidTexture = gpu::TexturePointer(gpu::Texture::create2D(gpu::Element(gpu::SCALAR, gpu::FLOAT, gpu::RGB), width, height, gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR_MIP_POINT)));
 //   _depthPyramidFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
 //   _depthPyramidFramebuffer->setRenderBuffer(0, _depthPyramidTexture);
  //  _depthPyramidFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

  //  resizeAmbientOcclusionBuffers();
}

/*
void DeferredFramebuffer::resizeAmbientOcclusionBuffers() {
    _occlusionFramebuffer.reset();
    _occlusionTexture.reset();
    _occlusionBlurredFramebuffer.reset();
    _occlusionBlurredTexture.reset();


    auto width = _frameBufferSize.width() >> _AOResolutionLevel;
    auto height = _frameBufferSize.height() >> _AOResolutionLevel;
    auto colorFormat = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGB);
    auto defaultSampler = gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_LINEAR);
    auto depthFormat = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::DEPTH_STENCIL); // Depth24_Stencil8 texel format

    _occlusionTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _occlusionFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _occlusionFramebuffer->setRenderBuffer(0, _occlusionTexture);
    _occlusionFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);

    _occlusionBlurredTexture = gpu::TexturePointer(gpu::Texture::create2D(colorFormat, width, height, defaultSampler));
    _occlusionBlurredFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create());
    _occlusionBlurredFramebuffer->setRenderBuffer(0, _occlusionBlurredTexture);
    _occlusionBlurredFramebuffer->setDepthStencilBuffer(_primaryDepthTexture, depthFormat);
}
*/


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
