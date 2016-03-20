//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusLegacyDisplayPlugin.h"

#include <memory>

#include <QtWidgets/QMainWindow>
#include <QtOpenGL/QGLWidget>
#include <GLMHelpers.h>
#include <QEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

#include <PerfStat.h>
#include <gl/OglplusHelpers.h>
#include <ViewFrustum.h>

#include "plugins/PluginContainer.h"
#include "OculusHelpers.h"

using namespace oglplus;

const QString OculusLegacyDisplayPlugin::NAME("Oculus Rift (0.5) (Legacy)");

OculusLegacyDisplayPlugin::OculusLegacyDisplayPlugin() {
}

void OculusLegacyDisplayPlugin::resetSensors() {
    ovrHmd_RecenterPose(_hmd);
}

void OculusLegacyDisplayPlugin::updateHeadPose(uint32_t frameIndex) {
    Lock lock(_mutex);
    _trackingState = ovrHmd_GetTrackingState(_hmd, ovr_GetTimeInSeconds());
    _headPoseCache.set(toGlm(_trackingState.HeadPose.ThePose));
}

bool OculusLegacyDisplayPlugin::isSupported() const {
    if (!ovr_Initialize(nullptr)) {
        return false;
    }
    bool result = false;
    if (ovrHmd_Detect() > 0) {
        result = true;
    }

    auto hmd = ovrHmd_Create(0);
    if (hmd) {
        QPoint targetPosition{ hmd->WindowsPos.x, hmd->WindowsPos.y };
        auto screens = qApp->screens();
        for(int i = 0; i < screens.size(); ++i) {
            auto screen = screens[i];
            QPoint position = screen->geometry().topLeft();
            if (position == targetPosition) {
                _hmdScreen = i;
                break;
            }
        }
    }
  
    ovr_Shutdown();
    return result;
}

void OculusLegacyDisplayPlugin::internalActivate() {
    Parent::internalActivate();
    
    if (!(ovr_Initialize(nullptr))) {
        Q_ASSERT(false);
        qFatal("Failed to Initialize SDK");
    }
    
    _hswDismissed = false;
    _hmd = ovrHmd_Create(0);
    if (!_hmd) {
        qFatal("Failed to acquire HMD");
    }
    
    _ipd = ovrHmd_GetFloat(_hmd, OVR_KEY_IPD, _ipd);

    glm::uvec2 eyeSizes[2];
    ovr_for_each_eye([&](ovrEyeType eye) {
        _eyeFovs[eye] = _hmd->MaxEyeFov[eye];
        ovrEyeRenderDesc erd = _eyeRenderDescs[eye] = ovrHmd_GetRenderDesc(_hmd, eye, _eyeFovs[eye]);
        ovrMatrix4f ovrPerspectiveProjection =
            ovrMatrix4f_Projection(erd.Fov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded);
        _eyeProjections[eye] = toGlm(ovrPerspectiveProjection);
        _eyeOffsets[eye] = glm::translate(mat4(), toGlm(erd.HmdToEyeViewOffset));
        eyeSizes[eye] = toGlm(ovrHmd_GetFovTextureSize(_hmd, eye, erd.Fov, 1.0f));
    });
    
    auto combinedFov = _eyeFovs[0];
    combinedFov.LeftTan = combinedFov.RightTan = std::max(combinedFov.LeftTan, combinedFov.RightTan);
    _cullingProjection = toGlm(ovrMatrix4f_Projection(combinedFov, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, ovrProjection_RightHanded));

    _renderTargetSize = uvec2(
        eyeSizes[0].x + eyeSizes[1].x,
        std::max(eyeSizes[0].y, eyeSizes[1].y));
    
    if (!ovrHmd_ConfigureTracking(_hmd,
                                  ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection, 0)) {
        qFatal("Could not attach to sensor device");
    }
}

void OculusLegacyDisplayPlugin::internalDeactivate() {
	Parent::internalDeactivate();
    ovrHmd_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
}

// DLL based display plugins MUST initialize GLEW inside the DLL code.
void OculusLegacyDisplayPlugin::customizeContext() {
    static std::once_flag once;
    std::call_once(once, []{
        glewExperimental = true;
        glewInit();
        glGetError();
    });
    Parent::customizeContext();
#if 0
    ovrGLConfig config; memset(&config, 0, sizeof(ovrRenderAPIConfig));
    auto& header = config.Config.Header;
    header.API = ovrRenderAPI_OpenGL;
    header.BackBufferSize = _hmd->Resolution;
    header.Multisample = 1;
    int distortionCaps = ovrDistortionCap_TimeWarp;
    
    memset(_eyeTextures, 0, sizeof(ovrTexture) * 2);
    ovr_for_each_eye([&](ovrEyeType eye) {
        auto& header = _eyeTextures[eye].Header;
        header.API = ovrRenderAPI_OpenGL;
        header.TextureSize = { (int)_desiredFramebufferSize.x, (int)_desiredFramebufferSize.y };
        header.RenderViewport.Size = header.TextureSize;
        header.RenderViewport.Size.w /= 2;
        if (eye == ovrEye_Right) {
            header.RenderViewport.Pos.x = header.RenderViewport.Size.w;
        }
    });
    
#ifndef NDEBUG
    ovrBool result =
#endif
    ovrHmd_ConfigureRendering(_hmd, &config.Config, distortionCaps, _eyeFovs, _eyeRenderDescs);
    assert(result);
#endif
    
}

#if 0
void OculusLegacyDisplayPlugin::uncustomizeContext() {
    HmdDisplayPlugin::uncustomizeContext();
}

void OculusLegacyDisplayPlugin::internalPresent() {
    ovrHmd_BeginFrame(_hmd, 0);
    ovr_for_each_eye([&](ovrEyeType eye) {
        reinterpret_cast<ovrGLTexture&>(_eyeTextures[eye]).OGL.TexId = _currentSceneTexture;
    });
    ovrHmd_EndFrame(_hmd, _eyePoses, _eyeTextures);
}

#endif

int OculusLegacyDisplayPlugin::getHmdScreen() const {
    return _hmdScreen;
}

float OculusLegacyDisplayPlugin::getTargetFrameRate() {
    return TARGET_RATE_OculusLegacy;
}


