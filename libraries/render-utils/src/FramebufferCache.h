//
//  Created by Bradley Austin Davis on 2015/07/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FramebufferCache_h
#define hifi_FramebufferCache_h

#include <QSize>

#include <gpu/Framebuffer.h>
#include <DependencyManager.h>

namespace gpu {
class Batch;
}

/// Stores cached textures, including render-to-texture targets.
class FramebufferCache : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    // Shadow map size is static
    static const int SHADOW_MAP_SIZE = 2048;

    /// Sets the desired texture resolution for the framebuffer objects. 
    void setFrameBufferSize(QSize frameBufferSize);
    const QSize& getFrameBufferSize() const { return _frameBufferSize; } 

    /// Returns a pointer to the primary framebuffer object.  This render target includes a depth component, and is
    /// used for scene rendering.
    gpu::FramebufferPointer getPrimaryFramebuffer();

    gpu::TexturePointer getPrimaryDepthTexture();
    gpu::TexturePointer getPrimaryColorTexture();

    gpu::FramebufferPointer getDeferredFramebuffer();
    gpu::FramebufferPointer getDeferredFramebufferDepthColor();

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
    

    gpu::TexturePointer getLightingTexture();
    gpu::FramebufferPointer getLightingFramebuffer();

    /// Returns the framebuffer object used to render selfie maps;
    gpu::FramebufferPointer getSelfieFramebuffer();

    /// Returns a free framebuffer with a single color attachment for temp or intra-frame operations
    gpu::FramebufferPointer getFramebuffer();

    // TODO add sync functionality to the release, so we don't reuse a framebuffer being read from
    /// Releases a free framebuffer back for reuse
    void releaseFramebuffer(const gpu::FramebufferPointer& framebuffer);

private:
    FramebufferCache();
    virtual ~FramebufferCache();

    void createPrimaryFramebuffer();

    gpu::FramebufferPointer _primaryFramebuffer;

    gpu::TexturePointer _primaryDepthTexture;
    gpu::TexturePointer _primaryColorTexture;

    gpu::FramebufferPointer _deferredFramebuffer;
    gpu::FramebufferPointer _deferredFramebufferDepthColor;

    gpu::TexturePointer _deferredColorTexture;
    gpu::TexturePointer _deferredNormalTexture;
    gpu::TexturePointer _deferredSpecularTexture;

    gpu::TexturePointer _lightingTexture;
    gpu::FramebufferPointer _lightingFramebuffer;

    gpu::FramebufferPointer _shadowFramebuffer;

    gpu::FramebufferPointer _selfieFramebuffer;

    gpu::FramebufferPointer _depthPyramidFramebuffer;
    gpu::TexturePointer _depthPyramidTexture;
    
    
    gpu::FramebufferPointer _occlusionFramebuffer;
    gpu::TexturePointer _occlusionTexture;
    
    gpu::FramebufferPointer _occlusionBlurredFramebuffer;
    gpu::TexturePointer _occlusionBlurredTexture;

    QSize _frameBufferSize{ 100, 100 };
    int _AOResolutionLevel = 1; // AO perform at half res

    // Resize/reallocate the buffers used for AO
    // the size of the AO buffers is scaled by the AOResolutionScale;
    void resizeAmbientOcclusionBuffers();
};

#endif // hifi_FramebufferCache_h
