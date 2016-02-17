//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenVrDisplayPlugin.h"

#include <memory>

#include <QMainWindow>
#include <QLoggingCategory>
#include <QGLWidget>
#include <QEvent>
#include <QResizeEvent>

#include <GLMHelpers.h>
#include <gl/GlWindow.h>

#include <PerfStat.h>
#include <plugins/PluginContainer.h>
#include <ViewFrustum.h>

#include "OpenVrHelpers.h"

Q_DECLARE_LOGGING_CATEGORY(displayplugins)

const QString OpenVrDisplayPlugin::NAME("OpenVR (Vive)");
const QString StandingHMDSensorMode = "Standing HMD Sensor Mode"; // this probably shouldn't be hardcoded here

static vr::IVRCompositor* _compositor{ nullptr };
static vr::TrackedDevicePose_t _presentThreadTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];
static mat4 _sensorResetMat;
static std::array<vr::Hmd_Eye, 2> VR_EYES { { vr::Eye_Left, vr::Eye_Right } };

bool OpenVrDisplayPlugin::isSupported() const {
    return vr::VR_IsHmdPresent();
}

void OpenVrDisplayPlugin::activate() {
    _container->setIsOptionChecked(StandingHMDSensorMode, true);

    if (!_hmd) {
        _hmd = acquireOpenVrSystem();
    }
    Q_ASSERT(_hmd);

    _hmd->GetRecommendedRenderTargetSize(&_renderTargetSize.x, &_renderTargetSize.y);
    // Recommended render target size is per-eye, so double the X size for 
    // left + right eyes
    _renderTargetSize.x *= 2;

    {
        Lock lock(_poseMutex);
        openvr_for_each_eye([&](vr::Hmd_Eye eye) {
            _eyeOffsets[eye] = toGlm(_hmd->GetEyeToHeadTransform(eye));
            _eyeProjections[eye] = toGlm(_hmd->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, vr::API_OpenGL));
        });
        // FIXME Calculate the proper combined projection by using GetProjectionRaw values from both eyes
        _cullingProjection = _eyeProjections[0];

    }

    _compositor = vr::VRCompositor();
    Q_ASSERT(_compositor);
    HmdDisplayPlugin::activate();
}

void OpenVrDisplayPlugin::deactivate() {
    _container->setIsOptionChecked(StandingHMDSensorMode, false);
    if (_hmd) {
        releaseOpenVrSystem();
        _hmd = nullptr;
    }
    _compositor = nullptr;
    HmdDisplayPlugin::deactivate();
}

void OpenVrDisplayPlugin::customizeContext() {
    // Display plugins in DLLs must initialize glew locally
    static std::once_flag once;
    std::call_once(once, []{
        glewExperimental = true;
        GLenum err = glewInit();
        glGetError();
    });
    HmdDisplayPlugin::customizeContext();
}

void OpenVrDisplayPlugin::resetSensors() {
    Lock lock(_poseMutex);
    glm::mat4 m = toGlm(_trackedDevicePose[0].mDeviceToAbsoluteTracking);
    _sensorResetMat = glm::inverse(cancelOutRollAndPitch(m));
}


glm::mat4 OpenVrDisplayPlugin::getHeadPose(uint32_t frameIndex) const {
    Lock lock(_poseMutex);
    return _trackedDevicePoseMat4[0];
}

void OpenVrDisplayPlugin::internalPresent() {
    // Flip y-axis since GL UV coords are backwards.
    static vr::VRTextureBounds_t leftBounds{ 0, 0, 0.5f, 1 };
    static vr::VRTextureBounds_t rightBounds{ 0.5f, 0, 1, 1 };

    vr::Texture_t texture{ (void*)_currentSceneTexture, vr::API_OpenGL, vr::ColorSpace_Auto };

    _compositor->Submit(vr::Eye_Left, &texture, &leftBounds);
    _compositor->Submit(vr::Eye_Right, &texture, &rightBounds);

    glFinish();

    _compositor->WaitGetPoses(_presentThreadTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

    {
        // copy and process _presentThreadTrackedDevicePoses
        Lock lock(_poseMutex);
        for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
            _trackedDevicePose[i] = _presentThreadTrackedDevicePose[i];
            _trackedDevicePoseMat4[i] = _sensorResetMat * toGlm(_trackedDevicePose[i].mDeviceToAbsoluteTracking);
        }
    }

    // Handle the mirroring in the base class
    HmdDisplayPlugin::internalPresent();
}
