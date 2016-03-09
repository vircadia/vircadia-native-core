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

void OculusBaseDisplayPlugin::resetSensors() {
    ovr_RecenterPose(_session);
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
    return oculusAvailable();
}

// DLL based display plugins MUST initialize GLEW inside the DLL code.
void OculusBaseDisplayPlugin::customizeContext() {
    glewExperimental = true;
    GLenum err = glewInit();
    glGetError();
    HmdDisplayPlugin::customizeContext();
}

void OculusBaseDisplayPlugin::init() {
}

void OculusBaseDisplayPlugin::deinit() {
}

void OculusBaseDisplayPlugin::activate() {
    _session = acquireOculusSession();

    _hmdDesc = ovr_GetHmdDesc(_session);

    _ipd = ovr_GetFloat(_session, OVR_KEY_IPD, _ipd);

    glm::uvec2 eyeSizes[2];
    _viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;

    ovr_for_each_eye([&](ovrEyeType eye) {
        _eyeFovs[eye] = _hmdDesc.DefaultEyeFov[eye];
        ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovr_GetRenderDesc(_session, eye, _eyeFovs[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded);
        _eyeProjections[eye] = toGlm(ovrPerspectiveProjection);
        _eyeOffsets[eye] = glm::translate(mat4(), toGlm(erd.HmdToEyeViewOffset));
        eyeSizes[eye] = toGlm(ovr_GetFovTextureSize(_session, eye, erd.Fov, 1.0f));
        _viewScaleDesc.HmdToEyeViewOffset[eye] = erd.HmdToEyeViewOffset;
    });

    auto combinedFov = _eyeFovs[0];
    combinedFov.LeftTan = combinedFov.RightTan = std::max(combinedFov.LeftTan, combinedFov.RightTan);
    _cullingProjection = toGlm(ovrMatrix4f_Projection(combinedFov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded));

    _renderTargetSize = uvec2(
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

    // This must come after the initialization, so that the values calculated 
    // above are available during the customizeContext call (when not running
    // in threaded present mode)
    HmdDisplayPlugin::activate();
}

void OculusBaseDisplayPlugin::deactivate() {
    HmdDisplayPlugin::deactivate();
    releaseOculusSession();
    _session = nullptr;
}
