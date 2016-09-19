//
//  Framebuffer.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 4/12/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Framebuffer_h
#define hifi_gpu_Framebuffer_h

#include "Texture.h"
#include <memory>

class Transform; // Texcood transform util

namespace gpu {

typedef Element Format;

class Swapchain {
public:
    // Properties
    uint16 getWidth() const { return _width; }
    uint16 getHeight() const { return _height; }
    uint16 getNumSamples() const { return _numSamples; }

    bool hasDepthStencil() const { return _hasDepthStencil; }
    bool isFullscreen() const { return _isFullscreen; }

    uint32 getSwapInterval() const { return _swapInterval; }

    bool isStereo() const { return _isStereo; }

    uint32 getFrameCount() const { return _frameCount; }

    // Pure interface
    void setSwapInterval(uint32 interval);

    void resize(uint16 width, uint16 height);
    void setFullscreen(bool fullscreen);

    Swapchain() {}
    Swapchain(const Swapchain& swapchain) {}
    virtual ~Swapchain() {}

protected:
    mutable uint32 _frameCount = 0;

    Format _colorFormat;
    uint16 _width = 1;
    uint16 _height = 1;
    uint16 _numSamples = 1;
    uint16 _swapInterval = 0;

    bool _hasDepthStencil = false;
    bool _isFullscreen = false;
    bool _isStereo = false;

    // Non exposed

    friend class Framebuffer;
};
typedef std::shared_ptr<Swapchain> SwapchainPointer;


class Framebuffer {
public:
    enum BufferMask {
        BUFFER_COLOR0 = 1,
        BUFFER_COLOR1 = 2,
        BUFFER_COLOR2 = 4,
        BUFFER_COLOR3 = 8,
        BUFFER_COLOR4 = 16,
        BUFFER_COLOR5 = 32,
        BUFFER_COLOR6 = 64,
        BUFFER_COLOR7 = 128,
        BUFFER_COLORS = 0x000000FF,

        BUFFER_DEPTH = 0x40000000,
        BUFFER_STENCIL = 0x80000000,
        BUFFER_DEPTHSTENCIL = 0xC0000000,
    };
    typedef uint32 Masks;

    ~Framebuffer();

    static Framebuffer* create(const SwapchainPointer& swapchain);
    static Framebuffer* create();
    static Framebuffer* create(const Format& colorBufferFormat, uint16 width, uint16 height);
    static Framebuffer* create(const Format& colorBufferFormat, const Format& depthStencilBufferFormat, uint16 width, uint16 height);
    static Framebuffer* createShadowmap(uint16 width);

    bool isSwapchain() const;
    SwapchainPointer getSwapchain() const { return _swapchain; }

    uint32 getFrameCount() const;

    // Render buffers
    void removeRenderBuffers();
    uint32 getNumRenderBuffers() const;
    const TextureViews& getRenderBuffers() const { return _renderBuffers; }

    int32 setRenderBuffer(uint32 slot, const TexturePointer& texture, uint32 subresource = 0);
    TexturePointer getRenderBuffer(uint32 slot) const;
    uint32 getRenderBufferSubresource(uint32 slot) const;

    bool setDepthStencilBuffer(const TexturePointer& texture, const Format& format, uint32 subresource = 0);
    TexturePointer getDepthStencilBuffer() const;
    uint32 getDepthStencilBufferSubresource() const;
    Format getDepthStencilBufferFormat() const;


    // Properties
    Masks getBufferMask() const { return _bufferMask; }
    bool isEmpty() const { return (_bufferMask == 0); }
    bool hasColor() const { return (getBufferMask() & BUFFER_COLORS); }
    bool hasDepthStencil() const { return (getBufferMask() & BUFFER_DEPTHSTENCIL); }
    bool hasDepth() const { return (getBufferMask() & BUFFER_DEPTH); }
    bool hasStencil() const { return (getBufferMask() & BUFFER_STENCIL); }

    bool validateTargetCompatibility(const Texture& texture, uint32 subresource = 0) const;

    Vec2u getSize() const { return Vec2u(getWidth(), getHeight()); }
    uint16 getWidth() const;
    uint16 getHeight() const;
    uint16 getNumSamples() const;

    float getAspectRatio() const { return getWidth() / (float) getHeight() ; }

    // If not a swapchain canvas, resize can resize all the render buffers and depth stencil attached in one call
    //void resize( uint16 width, uint16 height, uint16 samples = 1 );

    static const uint32 MAX_NUM_RENDER_BUFFERS = 8; 
    static uint32 getMaxNumRenderBuffers() { return MAX_NUM_RENDER_BUFFERS; }

    const GPUObjectPointer gpuObject {};

    Stamp getDepthStamp() const { return _depthStamp; }
    const std::vector<Stamp>& getColorStamps() const { return _colorStamps; }

    static glm::vec4 evalSubregionTexcoordTransformCoefficients(const glm::ivec2& sourceSurface, const glm::ivec2& destRegionSize, const glm::ivec2& destRegionOffset = glm::ivec2(0));
    static glm::vec4 evalSubregionTexcoordTransformCoefficients(const glm::ivec2& sourceSurface, const glm::ivec4& destViewport);

    static Transform evalSubregionTexcoordTransform(const glm::ivec2& sourceSurface, const glm::ivec2& destRegionSize, const glm::ivec2& destRegionOffset = glm::ivec2(0));
    static Transform evalSubregionTexcoordTransform(const glm::ivec2& sourceSurface, const glm::ivec4& destViewport);

protected:
    SwapchainPointer _swapchain;

    Stamp _depthStamp { 0 };
    std::vector<Stamp> _colorStamps;
    TextureViews _renderBuffers;
    TextureView _depthStencilBuffer;

    Masks _bufferMask = 0;

    uint32 _frameCount = 0;

    uint16 _width = 0;
    uint16 _height = 0;
    uint16 _numSamples = 0;

    void updateSize(const TexturePointer& texture);

    // Non exposed
    Framebuffer(const Framebuffer& framebuffer) = delete;
    Framebuffer() {}
};
typedef std::shared_ptr<Framebuffer> FramebufferPointer;

}

#endif
