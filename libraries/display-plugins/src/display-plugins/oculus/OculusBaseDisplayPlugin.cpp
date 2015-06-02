//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusBaseDisplayPlugin.h"

#include <ViewFrustum.h>

#include "OculusHelpers.h"

void OculusBaseDisplayPlugin::activate(PluginContainer * container) {
    glm::uvec2 eyeSizes[2];
    ovr_for_each_eye([&](ovrEyeType eye) {
        ovrEyeRenderDesc& erd = _eyeRenderDescs[eye] = ovrHmd_GetRenderDesc(_hmd, eye, _hmd->MaxEyeFov[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded);
        _eyeProjections[eye] = toGlm(ovrPerspectiveProjection);
        _eyeOffsets[eye] = erd.HmdToEyeViewOffset;
        eyeSizes[eye] = toGlm(ovrHmd_GetFovTextureSize(_hmd, eye, erd.Fov, 1.0f));
    });
    _desiredFramebufferSize = QSize(
        eyeSizes[0].x + eyeSizes[1].x,
        std::max(eyeSizes[0].y, eyeSizes[1].y));

    if (!OVR_SUCCESS(ovrHmd_ConfigureTracking(_hmd,
        ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0))) {
        qFatal("Could not attach to sensor device");
    }

    WidgetOpenGLDisplayPlugin::activate(container);
}

QSize OculusBaseDisplayPlugin::getRecommendedFramebufferSize() const {
    return _desiredFramebufferSize;
}

void OculusBaseDisplayPlugin::preRender() {
    ovrHmd_GetEyePoses(_hmd, _frameIndex, _eyeOffsets, _eyePoses, nullptr);
}

glm::mat4 OculusBaseDisplayPlugin::getProjection(Eye eye, const glm::mat4& baseProjection) const {
    return _eyeProjections[eye];
}

glm::mat4 OculusBaseDisplayPlugin::getModelview(Eye eye, const glm::mat4& baseModelview) const {
    return baseModelview * toGlm(_eyePoses[eye]);
}

void OculusBaseDisplayPlugin::resetSensors() {
    ovrHmd_RecenterPose(_hmd);
}