//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include <display-plugins/WindowOpenGLDisplayPlugin.h>

#include <QTimer>

#include <OVR_CAPI_GL.h>

class OculusBaseDisplayPlugin : public WindowOpenGLDisplayPlugin {
public:
    virtual bool isSupported() const override;

    virtual void init() override final;
    virtual void deinit() override final;

    virtual void activate() override;
    virtual void deactivate() override;

    // Stereo specific methods
    virtual bool isHmd() const override final { return true; }
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const override;
    virtual glm::uvec2 getRecommendedRenderSize() const override final;
    virtual glm::uvec2 getRecommendedUiSize() const override final { return uvec2(1920, 1080); }
    virtual void resetSensors() override final;
    virtual glm::mat4 getEyeToHeadTransform(Eye eye) const override final;
    virtual float getIPD() const override final;
    virtual glm::mat4 getHeadPose(uint32_t frameIndex) const override;

protected:
    virtual void customizeContext() override;

protected:
    ovrVector3f _eyeOffsets[2];
    
    mat4 _eyeProjections[3];
    mat4 _compositeEyeProjections[2];
    uvec2 _desiredFramebufferSize;

    ovrSession _session;
    ovrGraphicsLuid _luid;
    float _ipd{ OVR_DEFAULT_IPD };
    ovrEyeRenderDesc _eyeRenderDescs[2];
    ovrFovPort _eyeFovs[2];
    ovrHmdDesc _hmdDesc;
    ovrLayerEyeFov _sceneLayer;
};

#if (OVR_MAJOR_VERSION == 6) 
#define ovr_Create ovrHmd_Create
#define ovr_CreateSwapTextureSetGL ovrHmd_CreateSwapTextureSetGL
#define ovr_CreateMirrorTextureGL ovrHmd_CreateMirrorTextureGL 
#define ovr_Destroy ovrHmd_Destroy
#define ovr_DestroySwapTextureSet ovrHmd_DestroySwapTextureSet
#define ovr_DestroyMirrorTexture ovrHmd_DestroyMirrorTexture
#define ovr_GetFloat ovrHmd_GetFloat
#define ovr_GetFovTextureSize ovrHmd_GetFovTextureSize
#define ovr_GetFrameTiming ovrHmd_GetFrameTiming
#define ovr_GetTrackingState ovrHmd_GetTrackingState
#define ovr_GetRenderDesc ovrHmd_GetRenderDesc
#define ovr_RecenterPose ovrHmd_RecenterPose
#define ovr_SubmitFrame ovrHmd_SubmitFrame
#define ovr_ConfigureTracking ovrHmd_ConfigureTracking

#define ovr_GetHmdDesc(X) *X
#endif
