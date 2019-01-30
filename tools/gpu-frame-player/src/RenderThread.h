//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QtCore/QElapsedTimer>

#include <GenericThread.h>
#include <shared/RateCounter.h>

#ifdef USE_GL
#include <gl/Config.h>
#include <gl/Context.h>
#include <gpu/gl/GLBackend.h>
#else
#include <vk/VKWindow.h>
#include <gpu/vk/VKBackend.h>
#endif

class RenderThread : public GenericThread {
    using Parent = GenericThread;
public:
    QWindow* _window{ nullptr };

#ifdef USE_GL
    gl::Context _context;
#else
    vks::Context& _context{ vks::Context::get() };
    const vk::Device& _device{ _context.device };

    vk::SurfaceKHR _surface;
    vk::RenderPass _renderPass;
    vks::Swapchain _swapchain;
    vk::Semaphore acquireComplete, renderComplete;
    std::vector<vk::Framebuffer> _framebuffers;
    vk::Extent2D _extent;

    void setupFramebuffers();
    void setupRenderPass();
#endif

    std::mutex _mutex;
    gpu::ContextPointer _gpuContext;  // initialized during window creation
    std::shared_ptr<gpu::Backend> _backend;
    std::atomic<size_t> _presentCount{ 0 };
    QElapsedTimer _elapsed;
    size_t _frameIndex{ 0 };
    std::mutex _frameLock;
    std::queue<gpu::FramePointer> _pendingFrames;
    std::queue<QSize> _pendingSize;
    gpu::FramePointer _activeFrame;
    uint32_t _externalTexture{ 0 };


    void resize(const QSize& newSize);
    void setup() override;
    bool process() override;
    void shutdown() override;

    void submitFrame(const gpu::FramePointer& frame);
    void initialize(QWindow* window);
    void renderFrame(gpu::FramePointer& frame);
};
