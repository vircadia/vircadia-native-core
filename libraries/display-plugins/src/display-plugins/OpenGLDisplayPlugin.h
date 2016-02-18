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

#include <GLMHelpers.h>
#include <SimpleMovingAverage.h>
#include <gl/OglplusHelpers.h>
#include <gl/GLEscrow.h>

#define THREADED_PRESENT 1

class OpenGLDisplayPlugin : public DisplayPlugin {
protected:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using Condition = std::condition_variable;
public:
    OpenGLDisplayPlugin();
    virtual void activate() override;
    virtual void deactivate() override;
    virtual void stop() override;
    virtual bool eventFilter(QObject* receiver, QEvent* event) override;

    virtual void submitSceneTexture(uint32_t frameIndex, uint32_t sceneTexture, const glm::uvec2& sceneSize) override;
    virtual void submitOverlayTexture(uint32_t overlayTexture, const glm::uvec2& overlaySize) override;
    virtual float presentRate() override;

    virtual glm::uvec2 getRecommendedRenderSize() const override {
        return getSurfacePixels();
    }

    virtual glm::uvec2 getRecommendedUiSize() const override {
        return getSurfaceSize();
    }

    virtual QImage getScreenshot() const override;

protected:
#if THREADED_PRESENT
    friend class PresentThread;
#endif
    
    virtual glm::uvec2 getSurfaceSize() const = 0;
    virtual glm::uvec2 getSurfacePixels() const = 0;

    // FIXME make thread safe?
    virtual bool isVsyncEnabled();
    virtual void enableVsync(bool enable = true);

    // These functions must only be called on the presentation thread
    virtual void customizeContext();
    virtual void uncustomizeContext();
    virtual void cleanupForSceneTexture(uint32_t sceneTexture);
    void withMainThreadContext(std::function<void()> f) const;


    void present();
    void updateTextures();
    void updateFramerate();
    void drawUnitQuad();
    void swapBuffers();
    // Plugin specific functionality to composite the scene and overlay and present the result
    virtual void internalPresent();

    ProgramPtr _program;
    ShapeWrapperPtr _plane;

    mutable Mutex _mutex;
    SimpleMovingAverage _usecsPerFrame { 10 };
    QMap<uint32_t, uint32_t> _sceneTextureToFrameIndexMap;

    GLuint _currentSceneTexture { 0 };
    GLuint _currentOverlayTexture { 0 };

    GLTextureEscrow _sceneTextureEscrow;

    bool _vsyncSupported { false };

private:
#if THREADED_PRESENT
    void enableDeactivate();
    Condition _deactivateWait;
    bool _uncustomized{ false };
#endif

};


