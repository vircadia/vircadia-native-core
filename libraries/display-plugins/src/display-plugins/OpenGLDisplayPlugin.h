//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "DisplayPlugin.h"

#include <condition_variable>
#include <memory>
#include <queue>

#include <QtCore/QTimer>
#include <QtGui/QImage>

#include <GLMHelpers.h>
#include <SimpleMovingAverage.h>
#include <gl/GLEscrow.h>
#include <shared/RateCounter.h>

namespace gpu {
    namespace gl {
        class GLBackend;
    }
}

class OpenGLDisplayPlugin : public DisplayPlugin {
    Q_OBJECT
    Q_PROPERTY(float overlayAlpha MEMBER _overlayAlpha)
    using Parent = DisplayPlugin;
protected:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using Condition = std::condition_variable;
    using TextureEscrow = GLEscrow<gpu::TexturePointer>;
public:
    // These must be final to ensure proper ordering of operations 
    // between the main thread and the presentation thread
    bool activate() override final;
    void deactivate() override final;
    bool eventFilter(QObject* receiver, QEvent* event) override;
    bool isDisplayVisible() const override { return true; }

    void submitFrame(const gpu::FramePointer& newFrame) override;

    glm::uvec2 getRecommendedRenderSize() const override {
        return getSurfacePixels();
    }

    glm::uvec2 getRecommendedUiSize() const override {
        return getSurfaceSize();
    }

    QImage getScreenshot(float aspectRatio = 0.0f) const override;

    float presentRate() const override;

    float newFramePresentRate() const override;

    float droppedFrameRate() const override;

    float renderRate() const override;

    bool beginFrameRender(uint32_t frameIndex) override;

    virtual bool wantVsync() const { return true; }
    void setVsyncEnabled(bool vsyncEnabled) { _vsyncEnabled = vsyncEnabled; }
    bool isVsyncEnabled() const { return _vsyncEnabled; }

protected:
    friend class PresentThread;

    glm::uvec2 getSurfaceSize() const;
    glm::uvec2 getSurfacePixels() const;

    virtual void compositeLayers();
    virtual void compositeScene();
    virtual void compositeOverlay();
    virtual void compositePointer();
    virtual void compositeExtra() {};

    virtual bool hasFocus() const override;

    // These functions must only be called on the presentation thread
    virtual void customizeContext();
    virtual void uncustomizeContext();

    // Returns true on successful activation
    virtual bool internalActivate() { return true; }
    virtual void internalDeactivate() {}

    // Plugin specific functionality to send the composed scene to the output window or device
    virtual void internalPresent();

    virtual void updateFrameData();

    void withMainThreadContext(std::function<void()> f) const;

    void present();
    virtual void swapBuffers();
    ivec4 eyeViewport(Eye eye) const;

    void render(std::function<void(gpu::Batch& batch)> f);

    bool _vsyncEnabled { true };
    QThread* _presentThread{ nullptr };
    std::queue<gpu::FramePointer> _newFrameQueue;
    RateCounter<> _droppedFrameRate;
    RateCounter<> _newFrameRate;
    RateCounter<> _presentRate;
    RateCounter<> _renderRate;

    gpu::FramePointer _currentFrame;
    gpu::FramePointer _lastFrame;
    gpu::FramebufferPointer _compositeFramebuffer;
    gpu::PipelinePointer _overlayPipeline;
    gpu::PipelinePointer _simplePipeline;
    gpu::PipelinePointer _presentPipeline;
    gpu::PipelinePointer _cursorPipeline;
    float _compositeOverlayAlpha { 1.0f };

    struct CursorData {
        QImage image;
        vec2 hotSpot;
        uvec2 size;
        gpu::TexturePointer texture;
    };

    std::map<uint16_t, CursorData> _cursorsData;
    bool _lockCurrentTexture { false };

    void assertNotPresentThread() const;
    void assertIsPresentThread() const;

    template<typename F>
    void withPresentThreadLock(F f) const {
        assertIsPresentThread();
        Lock lock(_presentMutex);
        f();
    }

    template<typename F>
    void withNonPresentThreadLock(F f) const {
        assertNotPresentThread();
        Lock lock(_presentMutex);
        f();
    }

    gpu::gl::GLBackend* getGLBackend();

    // Any resource shared by the main thread and the presentation thread must
    // be serialized through this mutex
    mutable Mutex _presentMutex;
    float _overlayAlpha{ 1.0f };
};

