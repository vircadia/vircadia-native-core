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

namespace gpu {

typedef Element Format;

class Viewport {
public:
    int32  width = 1;
    int32  height = 1;
    int32   top = 0;
    int32   left = 0;
    float   zmin = 0.f;
    float   zmax = 1.f;

    Viewport() {}
    Viewport(int32 w, int32 h, int32 t = 0, int32 l = 0, float zin = 0.f, float zax = 1.f):
        width(w),
        height(h),
        top(t),
        left(l),
        zmin(zin),
        zmax(zax)
    {}
    Viewport(const Vec2i& wh, const Vec2i& tl = Vec2i(0), const Vec2& zrange = Vec2(0.f, 1.f)):
        width(wh.x),
        height(wh.y),
        top(tl.x),
        left(tl.y),
        zmin(zrange.x),
        zmax(zrange.y)
    {}
    ~Viewport() {}
};

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

    ~Framebuffer();

    static Framebuffer* create(const SwapchainPointer& swapchain);
    static Framebuffer* create();
    static Framebuffer* create(const Format& colorBufferFormat, const Format& depthStencilBufferFormat, uint16 width, uint16 height, uint16 samples );

    bool isSwapchain() const;
    SwapchainPointer getSwapchain() const { return _swapchain; }

    uint32 getFrameCount() const;

    // Render buffers
    int32 setRenderBuffer(uint32 slot, const TexturePointer& texture, uint32 subresource = 0);
    void removeRenderBuffers();
    uint32 getNumRenderBuffers() const;

    TexturePointer getRenderBuffer(uint32 slot) const;
    uint32 getRenderBufferSubresource(uint32 slot) const;

    bool setDepthStencilBuffer(const TexturePointer& texture, uint32 subresource = 0);
    TexturePointer getDepthStencilBuffer() const;
    uint32 getDepthStencilBufferSubresource() const;

    // Properties
    uint32 getBuffersMask() const { return _buffersMask; }
    bool isEmpty() const;

    bool validateTargetCompatibility(const Texture& texture, uint32 subresource = 0) const;

    uint16 getWidth() const;
    uint16 getHeight() const;
    uint16 getNumSamples() const;

    float getAspectRatio() const { return getWidth() / (float) getHeight() ; }

    // If not a swapchain canvas, resize can resize all the render buffers and depth stencil attached in one call
    void resize( uint16 width, uint16 height, uint16 samples = 1 );

    static const uint32 MAX_NUM_RENDER_BUFFERS = 8; 
    static uint32 getMaxNumRenderBuffers() { return MAX_NUM_RENDER_BUFFERS; }

    // Get viewport covering the ful Canvas
    Viewport getViewport() const { return Viewport(getWidth(), getHeight(), 0, 0); }


protected:

    Viewport _viewport;

    uint16 _width;
    uint16 _height;
    uint16 _numSamples;

    uint32 _buffersMask;

    uint32 _frameCount;

    SwapchainPointer _swapchain;

    Textures _renderBuffers;
    std::vector<uint32> _renderBuffersSubresource;

    TexturePointer _depthStencilBuffer;
    uint32 _depthStencilBufferSubresource;


    void updateSize(const TexturePointer& texture);

    // Non exposed
    Framebuffer(const Framebuffer& framebuffer) {}
    Framebuffer();
    
    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = NULL;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

}

#endif