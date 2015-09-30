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
    /// Sets the desired texture resolution for the framebuffer objects. 
    void setFrameBufferSize(QSize frameBufferSize);
    const QSize& getFrameBufferSize() const { return _frameBufferSize; } 

    /// Returns a pointer to the primary framebuffer object.  This render target includes a depth component, and is
    /// used for scene rendering.
    gpu::FramebufferPointer getPrimaryFramebuffer();
    gpu::FramebufferPointer getPrimaryFramebufferDepthColor();
    gpu::FramebufferPointer getPrimaryFramebufferStencilColor();

    gpu::TexturePointer getPrimaryDepthTexture();
    gpu::TexturePointer getPrimaryStencilTexture();
    gpu::TexturePointer getPrimaryColorTexture();
    gpu::TexturePointer getPrimaryNormalTexture();
    gpu::TexturePointer getPrimarySpecularTexture();

    /// Returns the framebuffer object used to render shadow maps;
    gpu::FramebufferPointer getShadowFramebuffer();

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

    gpu::FramebufferPointer _primaryFramebufferFull;
    gpu::FramebufferPointer _primaryFramebufferDepthColor;
    gpu::FramebufferPointer _primaryFramebufferStencilColor;
    gpu::TexturePointer _primaryDepthTexture;
    gpu::TexturePointer _primaryStencilTexture;
    gpu::TexturePointer _primaryColorTexture;
    gpu::TexturePointer _primaryNormalTexture;
    gpu::TexturePointer _primarySpecularTexture;
    
    gpu::FramebufferPointer _shadowFramebuffer;

    gpu::FramebufferPointer _selfieFramebuffer;

    QSize _frameBufferSize{ 100, 100 };
};

#endif // hifi_FramebufferCache_h
