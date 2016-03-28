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

#include <QtCore/QTimer>
#include <QtGui/QImage>

#include <GLMHelpers.h>
#include <SimpleMovingAverage.h>
#include <gl/OglplusHelpers.h>

#define THREADED_PRESENT 1
#include <gl/GLEscrow.h>

class OpenGLDisplayPlugin : public DisplayPlugin {
protected:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using Condition = std::condition_variable;
    using TextureEscrow = GLEscrow<gpu::TexturePointer>;
public:
    OpenGLDisplayPlugin();

    // These must be final to ensure proper ordering of operations 
    // between the main thread and the presentation thread
    void activate() override final;
    void deactivate() override final;

    bool eventFilter(QObject* receiver, QEvent* event) override;
    bool isDisplayVisible() const override { return true; }


    void submitSceneTexture(uint32_t frameIndex, const gpu::TexturePointer& sceneTexture) override;
    void submitOverlayTexture(const gpu::TexturePointer& overlayTexture) override;
    float presentRate() override;

    glm::uvec2 getRecommendedRenderSize() const override {
        return getSurfacePixels();
    }

    glm::uvec2 getRecommendedUiSize() const override {
        return getSurfaceSize();
    }

    QImage getScreenshot() const override;

protected:
#if THREADED_PRESENT
    friend class PresentThread;
#endif
    uint32_t getSceneTextureId() const;
    uint32_t getOverlayTextureId() const;

    glm::uvec2 getSurfaceSize() const;
    glm::uvec2 getSurfacePixels() const;

    void compositeLayers();
    virtual void compositeOverlay();
    virtual void compositePointer();

    virtual bool hasFocus() const override;

    // FIXME make thread safe?
    virtual bool isVsyncEnabled();
    virtual void enableVsync(bool enable = true);

    // These functions must only be called on the presentation thread
    virtual void customizeContext();
    virtual void uncustomizeContext();

    virtual void internalActivate() {}
    virtual void internalDeactivate() {}
    virtual void cleanupForSceneTexture(const gpu::TexturePointer& sceneTexture);
    // Plugin specific functionality to send the composed scene to the output window or device
    virtual void internalPresent();

    void withMainThreadContext(std::function<void()> f) const;

    void present();
    void updateTextures();
    void updateFramerate();
    void drawUnitQuad();
    void swapBuffers();
    void eyeViewport(Eye eye) const;

    virtual void updateFrameData();

    ProgramPtr _program;
    int32_t _mvpUniform { -1 };
    int32_t _alphaUniform { -1 };
    ShapeWrapperPtr _plane;

    mutable Mutex _mutex;
    SimpleMovingAverage _usecsPerFrame { 10 };
    QMap<gpu::TexturePointer, uint32_t> _sceneTextureToFrameIndexMap;
    uint32_t _currentRenderFrameIndex { 0 };

    gpu::TexturePointer _currentSceneTexture;
    gpu::TexturePointer _currentOverlayTexture;

    TextureEscrow _sceneTextureEscrow;
    TextureEscrow _overlayTextureEscrow;

    bool _vsyncSupported { false };

    struct CursorData {
        QImage image;
        vec2 hotSpot;
        uvec2 size;
        uint32_t texture { 0 };
    };

    std::map<uint16_t, CursorData> _cursorsData;
    BasicFramebufferWrapperPtr _compositeFramebuffer;
};


