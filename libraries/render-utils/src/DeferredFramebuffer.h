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

#ifndef hifi_DeferredFramebuffer_h
#define hifi_DeferredFramebuffer_h

#include "gpu/Resource.h"
#include "gpu/Framebuffer.h"

class RenderArgs;

// DeferredFramebuffer is  a helper class gathering in one place the GBuffer (Framebuffer) and lighting framebuffer
class DeferredFramebuffer {
public:
    DeferredFramebuffer();

    gpu::FramebufferPointer getDeferredFramebuffer();
    gpu::FramebufferPointer getDeferredFramebufferDepthColor();

    gpu::TexturePointer getPrimaryDepthTexture();

    gpu::TexturePointer getDeferredColorTexture();
    gpu::TexturePointer getDeferredNormalTexture();
    gpu::TexturePointer getDeferredSpecularTexture();

    gpu::FramebufferPointer getDepthPyramidFramebuffer();
    gpu::TexturePointer getDepthPyramidTexture();


    void setAmbientOcclusionResolutionLevel(int level);
    gpu::FramebufferPointer getOcclusionFramebuffer();
    gpu::TexturePointer getOcclusionTexture();
    gpu::FramebufferPointer getOcclusionBlurredFramebuffer();
    gpu::TexturePointer getOcclusionBlurredTexture();


    gpu::FramebufferPointer getLightingFramebuffer();
    gpu::TexturePointer getLightingTexture();

    void setPrimaryDepth(const gpu::TexturePointer& depthBuffer);

    void setFrameSize(const glm::ivec2& size);
    const glm::ivec2& getFrameSize() const { return _frameSize; }


protected:
    void allocate();

    gpu::TexturePointer _primaryDepthTexture;

    gpu::FramebufferPointer _deferredFramebuffer;
    gpu::FramebufferPointer _deferredFramebufferDepthColor;

    gpu::TexturePointer _deferredColorTexture;
    gpu::TexturePointer _deferredNormalTexture;
    gpu::TexturePointer _deferredSpecularTexture;

    gpu::TexturePointer _lightingTexture;
    gpu::FramebufferPointer _lightingFramebuffer;

    gpu::FramebufferPointer _occlusionFramebuffer;
    gpu::TexturePointer _occlusionTexture;

    gpu::FramebufferPointer _occlusionBlurredFramebuffer;
    gpu::TexturePointer _occlusionBlurredTexture;

    glm::ivec2 _frameSize;

};

using DeferredFramebufferPointer = std::shared_ptr<DeferredFramebuffer>;

#endif // hifi_DeferredFramebuffer_h