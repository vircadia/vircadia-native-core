//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusBaseDisplayPlugin.h"

#include <ViewFrustum.h>

#include "OculusHelpers.h"

uvec2 OculusBaseDisplayPlugin::getRecommendedRenderSize() const {
    return _desiredFramebufferSize;
}

void OculusBaseDisplayPlugin::preRender() {
#if (OVR_MAJOR_VERSION >= 6)
    ovrFrameTiming ftiming = ovr_GetFrameTiming(_hmd, _frameIndex);
    _trackingState = ovr_GetTrackingState(_hmd, ftiming.DisplayMidpointSeconds);
    ovr_CalcEyePoses(_trackingState.HeadPose.ThePose, _eyeOffsets, _eyePoses);
#endif
}

glm::mat4 OculusBaseDisplayPlugin::getProjection(Eye eye, const glm::mat4& baseProjection) const {
    return _eyeProjections[eye];
}

void OculusBaseDisplayPlugin::resetSensors() {
#if (OVR_MAJOR_VERSION >= 6)
    ovr_RecenterPose(_hmd);
#endif
}

glm::mat4 OculusBaseDisplayPlugin::getEyePose(Eye eye) const {
    return toGlm(_eyePoses[eye]);
}

glm::mat4 OculusBaseDisplayPlugin::getHeadPose() const {
    return toGlm(_trackingState.HeadPose.ThePose);
}

bool OculusBaseDisplayPlugin::isSupported() const {
#if (OVR_MAJOR_VERSION >= 6)
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        return false;
    }
    bool result = false;
    if (ovrHmd_Detect() > 0) {
        result = true;
    }
    ovr_Shutdown();
    return result;
#else
    return false;
#endif
}

void OculusBaseDisplayPlugin::init() {
}

void OculusBaseDisplayPlugin::deinit() {
}

void OculusBaseDisplayPlugin::activate() {
#if (OVR_MAJOR_VERSION >= 6)
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        qFatal("Could not init OVR");
    }

#if (OVR_MAJOR_VERSION == 6)
    if (!OVR_SUCCESS(ovr_Create(0, &_hmd))) {
#elif (OVR_MAJOR_VERSION == 7)
    if (!OVR_SUCCESS(ovr_Create(&_hmd, &_luid))) {
#endif
        Q_ASSERT(false);
        qFatal("Failed to acquire HMD");
    }

    _hmdDesc = ovr_GetHmdDesc(_hmd);

    _ipd = ovr_GetFloat(_hmd, OVR_KEY_IPD, _ipd);

    glm::uvec2 eyeSizes[2];
    ovr_for_each_eye([&](ovrEyeType eye) {
        _eyeFovs[eye] = _hmdDesc.DefaultEyeFov[eye];
        ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_hmd, eye, _eyeFovs[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded);
        _eyeProjections[eye] = toGlm(ovrPerspectiveProjection);

        ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, 0.001f, 10.0f, ovrProjection_RightHanded);
        _compositeEyeProjections[eye] = toGlm(ovrPerspectiveProjection);

        _eyeOffsets[eye] = erd.HmdToEyeViewOffset;
        eyeSizes[eye] = toGlm(ovr_GetFovTextureSize(_hmd, eye, erd.Fov, 1.0f));
    });
    ovrFovPort combined = _eyeFovs[Left];
    combined.LeftTan = std::max(_eyeFovs[Left].LeftTan, _eyeFovs[Right].LeftTan);
    combined.RightTan = std::max(_eyeFovs[Left].RightTan, _eyeFovs[Right].RightTan);
    ovrMatrix4f ovrPerspectiveProjection =
        ovrMatrix4f_Projection(combined, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded);
    _eyeProjections[Mono] = toGlm(ovrPerspectiveProjection);



    _desiredFramebufferSize = uvec2(
        eyeSizes[0].x + eyeSizes[1].x,
        std::max(eyeSizes[0].y, eyeSizes[1].y));

    _frameIndex = 0;

    if (!OVR_SUCCESS(ovr_ConfigureTracking(_hmd,
        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
        qFatal("Could not attach to sensor device");
    }

    // Parent class relies on our _hmd intialization, so it must come after that.
    memset(&_sceneLayer, 0, sizeof(ovrLayerEyeFov));
    _sceneLayer.Header.Type = ovrLayerType_EyeFov;
    _sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrFovPort & fov = _sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
        ovrSizei & size = _sceneLayer.Viewport[eye].Size = ovr_GetFovTextureSize(_hmd, eye, fov, 1.0f);
        _sceneLayer.Viewport[eye].Pos = { eye == ovrEye_Left ? 0 : size.w, 0 };
    });

    if (!OVR_SUCCESS(ovr_ConfigureTracking(_hmd,
        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
        qFatal("Could not attach to sensor device");
    }
#endif

    WindowOpenGLDisplayPlugin::activate();
}

void OculusBaseDisplayPlugin::deactivate() {
    WindowOpenGLDisplayPlugin::deactivate();

#if (OVR_MAJOR_VERSION >= 6)
    ovr_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
#endif
}

void OculusBaseDisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    ++_frameIndex;
}

float OculusBaseDisplayPlugin::getIPD() const {
    float result = 0.0f;
#if (OVR_MAJOR_VERSION >= 6)
    result = ovr_GetFloat(_hmd, OVR_KEY_IPD, OVR_DEFAULT_IPD);
#endif
    return result;
}