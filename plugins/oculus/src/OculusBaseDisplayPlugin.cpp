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

glm::mat4 OculusBaseDisplayPlugin::getProjection(Eye eye, const glm::mat4& baseProjection) const {
    return _eyeProjections[eye];
}

void OculusBaseDisplayPlugin::resetSensors() {
    ovr_RecenterPose(_session);
}

glm::mat4 OculusBaseDisplayPlugin::getEyeToHeadTransform(Eye eye) const {
    return glm::translate(mat4(), toGlm(_eyeOffsets[eye]));
}

glm::mat4 OculusBaseDisplayPlugin::getHeadPose(uint32_t frameIndex) const {
    static uint32_t lastFrameSeen = 0;
    auto displayTime = ovr_GetPredictedDisplayTime(_session, frameIndex);
    auto trackingState = ovr_GetTrackingState(_session, displayTime, frameIndex > lastFrameSeen);
    if (frameIndex > lastFrameSeen) {
        lastFrameSeen = frameIndex;
    }
    return toGlm(trackingState.HeadPose.ThePose);
}

bool OculusBaseDisplayPlugin::isSupported() const {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        return false;
    }

    ovrSession session { nullptr };
    ovrGraphicsLuid luid;
    auto result = ovr_Create(&session, &luid);
    if (!OVR_SUCCESS(result)) {
        ovrErrorInfo error;
        ovr_GetLastErrorInfo(&error);
        ovr_Shutdown();
        return false;
    }

    auto hmdDesc = ovr_GetHmdDesc(session);
    if (hmdDesc.Type == ovrHmd_None) {
        ovr_Destroy(session);
        ovr_Shutdown();
        return false;
    }

    ovr_Shutdown();
    return true;
}

// DLL based display plugins MUST initialize GLEW inside the DLL code.
void OculusBaseDisplayPlugin::customizeContext() {
    glewExperimental = true;
    GLenum err = glewInit();
    glGetError();
    WindowOpenGLDisplayPlugin::customizeContext();
}

void OculusBaseDisplayPlugin::init() {
}

void OculusBaseDisplayPlugin::deinit() {
}

void OculusBaseDisplayPlugin::activate() {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        qFatal("Could not init OVR");
    }

    if (!OVR_SUCCESS(ovr_Create(&_session, &_luid))) {
        qFatal("Failed to acquire HMD");
    }

    WindowOpenGLDisplayPlugin::activate();

    _hmdDesc = ovr_GetHmdDesc(_session);

    _ipd = ovr_GetFloat(_session, OVR_KEY_IPD, _ipd);

    glm::uvec2 eyeSizes[2];
    ovr_for_each_eye([&](ovrEyeType eye) {
        _eyeFovs[eye] = _hmdDesc.DefaultEyeFov[eye];
        ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_session, eye, _eyeFovs[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded);
        _eyeProjections[eye] = toGlm(ovrPerspectiveProjection);

        ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, 0.001f, 10.0f, ovrProjection_RightHanded);
        _compositeEyeProjections[eye] = toGlm(ovrPerspectiveProjection);

        _eyeOffsets[eye] = erd.HmdToEyeViewOffset;
        eyeSizes[eye] = toGlm(ovr_GetFovTextureSize(_session, eye, erd.Fov, 1.0f));
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

    if (!OVR_SUCCESS(ovr_ConfigureTracking(_session,
        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
        qFatal("Could not attach to sensor device");
    }

    // Parent class relies on our _session intialization, so it must come after that.
    memset(&_sceneLayer, 0, sizeof(ovrLayerEyeFov));
    _sceneLayer.Header.Type = ovrLayerType_EyeFov;
    _sceneLayer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrFovPort & fov = _sceneLayer.Fov[eye] = _eyeRenderDescs[eye].Fov;
        ovrSizei & size = _sceneLayer.Viewport[eye].Size = ovr_GetFovTextureSize(_session, eye, fov, 1.0f);
        _sceneLayer.Viewport[eye].Pos = { eye == ovrEye_Left ? 0 : size.w, 0 };
    });

    if (!OVR_SUCCESS(ovr_ConfigureTracking(_session,
        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
        qFatal("Could not attach to sensor device");
    }
}

void OculusBaseDisplayPlugin::deactivate() {
    WindowOpenGLDisplayPlugin::deactivate();

#if (OVR_MAJOR_VERSION >= 6)
    ovr_Destroy(_session);
    _session = nullptr;
    ovr_Shutdown();
#endif
}


float OculusBaseDisplayPlugin::getIPD() const {
    float result = OVR_DEFAULT_IPD;
#if (OVR_MAJOR_VERSION >= 6)
    result = ovr_GetFloat(_session, OVR_KEY_IPD, result);
#endif
    return result;
}
