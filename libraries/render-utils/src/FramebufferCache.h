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

#include <mutex>
#include <gpu/Forward.h>
#include <DependencyManager.h>

/// Stores cached textures, including render-to-texture targets.
class FramebufferCache : public Dependency {
    SINGLETON_DEPENDENCY
    
public:
    // Shadow map size is static
    static const int SHADOW_MAP_SIZE = 2048;

    /// Sets the desired texture resolution for the framebuffer objects. 
    void setFrameBufferSize(QSize frameBufferSize);
    const QSize& getFrameBufferSize() const { return _frameBufferSize; } 

    /// Returns the framebuffer object used to render selfie maps;
    gpu::FramebufferPointer getSelfieFramebuffer();

    /// Returns a free framebuffer with a single color attachment for temp or intra-frame operations
    gpu::FramebufferPointer getFramebuffer();

    // TODO add sync functionality to the release, so we don't reuse a framebuffer being read from
    /// Releases a free framebuffer back for reuse
    void releaseFramebuffer(const gpu::FramebufferPointer& framebuffer);

private:
    void createPrimaryFramebuffer();

    gpu::FramebufferPointer _shadowFramebuffer;

    gpu::FramebufferPointer _selfieFramebuffer;

    QSize _frameBufferSize{ 100, 100 };

    std::mutex _mutex;
    std::list<gpu::FramebufferPointer> _cachedFramebuffers;
};

#endif // hifi_FramebufferCache_h
