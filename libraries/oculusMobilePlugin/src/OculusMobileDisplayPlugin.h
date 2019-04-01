//
//  Created by Bradley Austin Davis on 2018/11/15
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <display-plugins/hmd/HmdDisplayPlugin.h>

#include <EGL/egl.h>

#include <QTimer>
#include <QtPlatformHeaders/QEGLNativeContext>
#include <QtAndroidExtras/QAndroidJniObject>

#include <gl/Context.h>
#include <ovr/VrHandler.h>

typedef struct ovrTextureSwapChain ovrTextureSwapChain;
typedef struct ovrMobile ovrMobile;
typedef struct ANativeWindow ANativeWindow;

class OculusMobileDisplayPlugin : public HmdDisplayPlugin, public ovr::VrHandler {
    using Parent = HmdDisplayPlugin;
public:
    OculusMobileDisplayPlugin();
    virtual ~OculusMobileDisplayPlugin();
    bool isSupported() const override { return true; };
    bool hasAsyncReprojection() const override { return true; }
    bool getSupportsAutoSwitch() override final { return false; }
    QThread::Priority getPresentPriority() override { return QThread::TimeCriticalPriority; }

    glm::mat4 getEyeProjection(Eye eye, const glm::mat4& baseProjection) const override;
    glm::mat4 getCullingProjection(const glm::mat4& baseProjection) const override;

    // Stereo specific methods
    void resetSensors() override final;
    bool beginFrameRender(uint32_t frameIndex) override;

    QRectF getPlayAreaRect() override;
    float getTargetFrameRate() const override;
    void init() override;
    void deinit() override;

protected:
    const QString getName() const override { return NAME; }

    bool internalActivate() override;
    void internalDeactivate() override;

    void customizeContext() override;
    void uncustomizeContext() override;

    void updatePresentPose() override;
    void internalPresent(const gpu::FramebufferPointer&) override;
    void hmdPresent(const gpu::FramebufferPointer&) override { throw std::runtime_error("Unused"); }
    bool isHmdMounted() const override;
    bool alwaysPresent() const override { return true; }

    static const char* NAME;
    mutable gl::Context* _mainContext{ nullptr };
    uint32_t _readFbo;
};

