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

#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QImage>

#include <GLMHelpers.h>
#include <SimpleMovingAverage.h>
#include <shared/RateCounter.h>

#include <gpu/Batch.h>

namespace gpu {
    namespace gl {
        class GLBackend;
    }
}

class RefreshRateController;

class OpenGLDisplayPlugin : public DisplayPlugin {
    Q_OBJECT
    Q_PROPERTY(float hudAlpha MEMBER _hudAlpha)
    using Parent = DisplayPlugin;
protected:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using Condition = std::condition_variable;
public:
    ~OpenGLDisplayPlugin();
    // These must be final to ensure proper ordering of operations
    // between the main thread and the presentation thread

    static std::function<void(int)> getRefreshRateOperator();

    bool activate() override final;
    void deactivate() override final;
    bool startStandBySession() override final;
    void endSession() override final;
    bool eventFilter(QObject* receiver, QEvent* event) override;
    bool isDisplayVisible() const override { return true; }
    void captureFrame(const std::string& outputName) const override;
    void submitFrame(const gpu::FramePointer& newFrame) override;

    glm::uvec2 getRecommendedRenderSize() const override {
        return getSurfacePixels();
    }

    glm::uvec2 getRecommendedUiSize() const override {
        return getSurfaceSize();
    }

    virtual bool setDisplayTexture(const QString& name) override;
    virtual bool onDisplayTextureReset() { return false; };

    float presentRate() const override;

    void resetPresentRate() override;

    float newFramePresentRate() const override;

    float droppedFrameRate() const override;

    float renderRate() const override;

    bool beginFrameRender(uint32_t frameIndex) override;

    virtual bool wantVsync() const { return true; }
    void setVsyncEnabled(bool vsyncEnabled) { _vsyncEnabled = vsyncEnabled; }
    bool isVsyncEnabled() const { return _vsyncEnabled; }
    // Three threads, one for rendering, one for texture transfers, one reserved for the GL driver
    int getRequiredThreadCount() const override { return 3; }

    void copyTextureToQuickFramebuffer(NetworkTexturePointer source, QOpenGLFramebufferObject* target, GLsync* fenceSync) override;

    virtual std::function<void(gpu::Batch&, const gpu::TexturePointer&, bool mirror)> getHUDOperator() override;

protected:
    friend class PresentThread;

    glm::uvec2 getSurfaceSize() const;
    glm::uvec2 getSurfacePixels() const;
    // Some display plugins require us to always execute some present logic,
    // whether we have a frame or not (Oculus Mobile plugin)
    // Such plugins must be prepared to do the right thing if the `_currentFrame`
    // is not populated
    virtual bool alwaysPresent() const { return false; }

    void updateCompositeFramebuffer();

    virtual QThread::Priority getPresentPriority() { return QThread::HighPriority; }
    virtual void compositeLayers();
    virtual void compositeScene();
    virtual void compositePointer();
    virtual void compositeExtra() {};

    virtual gpu::PipelinePointer getCompositeScenePipeline();
    virtual gpu::Element getCompositeFBColorSpace();

    // These functions must only be called on the presentation thread
    virtual void customizeContext();
    virtual void uncustomizeContext();

    // Returns true on successful activation
    virtual bool internalActivate() { return true; }
    virtual void internalDeactivate() {}

    // Returns true on successful activation of standby session
    virtual bool activateStandBySession() { return true; }
    virtual void deactivateSession() {}

    // Plugin specific functionality to send the composed scene to the output window or device
    virtual void internalPresent();

    void renderFromTexture(gpu::Batch& batch, const gpu::TexturePointer& texture, const glm::ivec4& viewport, const glm::ivec4& scissor, const gpu::FramebufferPointer& fbo);
    void renderFromTexture(gpu::Batch& batch, const gpu::TexturePointer& texture, const glm::ivec4& viewport, const glm::ivec4& scissor);
    virtual void updateFrameData();
    virtual glm::mat4 getViewCorrection() { return glm::mat4(); }

    void withOtherThreadContext(std::function<void()> f) const;

    void present(const std::shared_ptr<RefreshRateController>& refreshRateController);
    virtual void swapBuffers();
    ivec4 eyeViewport(Eye eye) const;

    void render(std::function<void(gpu::Batch& batch)> f);

    bool _vsyncEnabled { true };
    QThread* _presentThread{ nullptr };
    std::queue<gpu::FramePointer> _newFrameQueue;
    RateCounter<200> _droppedFrameRate;
    RateCounter<200> _newFrameRate;
    RateCounter<200> _presentRate;
    RateCounter<200> _renderRate;

    gpu::FramePointer _currentFrame;
    gpu::Frame* _lastFrame { nullptr };
    mat4 _prevRenderView;
    gpu::FramebufferPointer _compositeFramebuffer;
    gpu::PipelinePointer _hudPipeline;
    gpu::PipelinePointer _mirrorHUDPipeline;
    gpu::ShaderPointer _mirrorHUDPS;
    gpu::PipelinePointer _drawTexturePipeline;
    gpu::PipelinePointer _linearToSRGBPipeline;
    gpu::PipelinePointer _SRGBToLinearPipeline;
    gpu::PipelinePointer _cursorPipeline;

    gpu::TexturePointer _displayTexture{};
    float _compositeHUDAlpha { 1.0f };

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
    float _hudAlpha{ 1.0f };

    QImage getScreenshot(float aspectRatio);
    QImage getSecondaryCameraScreenshot();
};

