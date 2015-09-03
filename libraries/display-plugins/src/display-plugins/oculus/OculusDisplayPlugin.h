//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "../WindowOpenGLDisplayPlugin.h"

#include <QTimer>

#include <OVR_CAPI.h>

class OffscreenGlCanvas;
struct SwapFramebufferWrapper;
struct MirrorFramebufferWrapper;

using SwapFboPtr = QSharedPointer<SwapFramebufferWrapper>;
using MirrorFboPtr = QSharedPointer<MirrorFramebufferWrapper>;

class OculusDisplayPlugin : public WindowOpenGLDisplayPlugin {
public:
    virtual bool isSupported() const override;
    virtual const QString & getName() const override;

    virtual void init() override;
    virtual void deinit() override;

    virtual void activate() override;
    virtual void deactivate() override;

    virtual bool eventFilter(QObject* receiver, QEvent* event) override;

    // Stereo specific methods
    virtual bool isHmd() const override { return true; }
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const override;
    virtual glm::uvec2 getRecommendedRenderSize() const override;
    virtual glm::uvec2 getRecommendedUiSize() const override { return uvec2(1920, 1080); }
    virtual void resetSensors() override;
    virtual glm::mat4 getEyePose(Eye eye) const override;
    virtual glm::mat4 getHeadPose() const override;

protected:
    virtual void preRender() override;
    virtual void display(GLuint finalTexture, const glm::uvec2& sceneSize) override;
    virtual void customizeContext() override;
    // Do not perform swap in finish
    virtual void finishFrame() override;

private:
    static const QString NAME;
    
    ovrPosef _eyePoses[2];
    
    mat4 _eyeProjections[3];
    mat4 _compositeEyeProjections[2];
    uvec2 _desiredFramebufferSize;
    ovrTrackingState _trackingState;
    bool _enableMirror{ false };

#if (OVR_MAJOR_VERSION >= 6)
    ovrHmd _hmd;
    float _ipd{ OVR_DEFAULT_IPD };
    unsigned int _frameIndex;
    ovrEyeRenderDesc _eyeRenderDescs[2];
    ovrVector3f _eyeOffsets[2];
    ovrFovPort _eyeFovs[2];
    
    ovrLayerEyeFov&  getSceneLayer();
    ovrHmdDesc       _hmdDesc;
    SwapFboPtr       _sceneFbo;
    ovrLayerEyeFov   _sceneLayer;
#endif
#if (OVR_MAJOR_VERSION == 7)
    ovrGraphicsLuid  _luid;
#endif
};

