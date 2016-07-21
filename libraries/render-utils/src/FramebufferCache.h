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

    void setAmbientOcclusionResolutionLevel(int level);
    gpu::FramebufferPointer getOcclusionFramebuffer();
    gpu::TexturePointer getOcclusionTexture();
    gpu::FramebufferPointer getOcclusionBlurredFramebuffer();
    gpu::TexturePointer getOcclusionBlurredTexture();

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

    gpu::FramebufferPointer _shadowFramebuffer;

    gpu::FramebufferPointer _selfieFramebuffer;

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
