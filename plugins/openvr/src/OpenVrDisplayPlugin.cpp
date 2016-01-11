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
Q_LOGGING_CATEGORY(displayplugins, "hifi.displayplugins")

const QString OpenVrDisplayPlugin::NAME("OpenVR (Vive)");

const QString StandingHMDSensorMode = "Standing HMD Sensor Mode"; // this probably shouldn't be hardcoded here

static vr::IVRCompositor* _compositor{ nullptr };
vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];
static mat4 _sensorResetMat;
static uvec2 _windowSize;
static uvec2 _renderTargetSize;

struct PerEyeData {
    //uvec2 _viewportOrigin;
    //uvec2 _viewportSize;
    mat4 _projectionMatrix;
    mat4 _eyeOffset;
    mat4 _pose;
};

static PerEyeData _eyesData[2];


template<typename F>
void openvr_for_each_eye(F f) {
    f(vr::Hmd_Eye::Eye_Left);
    f(vr::Hmd_Eye::Eye_Right);
}

mat4 toGlm(const vr::HmdMatrix44_t& m) {
    return glm::transpose(glm::make_mat4(&m.m[0][0]));
}

mat4 toGlm(const vr::HmdMatrix34_t& m) {
    mat4 result = mat4(
        m.m[0][0], m.m[1][0], m.m[2][0], 0.0,
        m.m[0][1], m.m[1][1], m.m[2][1], 0.0,
        m.m[0][2], m.m[1][2], m.m[2][2], 0.0,
        m.m[0][3], m.m[1][3], m.m[2][3], 1.0f);
    return result;
}

bool OpenVrDisplayPlugin::isSupported() const {
    auto hmd = acquireOpenVrSystem();
    bool success = nullptr != hmd;
    releaseOpenVrSystem();
    return success;
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
    openvr_for_each_eye([&](vr::Hmd_Eye eye) {
        PerEyeData& eyeData = _eyesData[eye];
        eyeData._projectionMatrix = toGlm(_hmd->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, vr::API_OpenGL));
        eyeData._eyeOffset = toGlm(_hmd->GetEyeToHeadTransform(eye));
    });
    _compositor = vr::VRCompositor();
    Q_ASSERT(_compositor);
    WindowOpenGLDisplayPlugin::activate();
}

void OpenVrDisplayPlugin::deactivate() {
    _container->setIsOptionChecked(StandingHMDSensorMode, false);
    if (_hmd) {
        releaseOpenVrSystem();
        _hmd = nullptr;
    }
    _compositor = nullptr;
    WindowOpenGLDisplayPlugin::deactivate();
}

void OpenVrDisplayPlugin::customizeContext() {
    static std::once_flag once;
    std::call_once(once, []{
        glewExperimental = true;
        GLenum err = glewInit();
        glGetError();
    });
    WindowOpenGLDisplayPlugin::customizeContext();
}

uvec2 OpenVrDisplayPlugin::getRecommendedRenderSize() const {
    return _renderTargetSize;
}

mat4 OpenVrDisplayPlugin::getProjection(Eye eye, const mat4& baseProjection) const {
    // FIXME hack to ensure that we don't crash trying to get the combined matrix
    if (eye == Mono) {
        eye = Left;
    }
    return _eyesData[eye]._projectionMatrix;
}

void OpenVrDisplayPlugin::resetSensors() {
    _sensorResetMat = glm::inverse(cancelOutRollAndPitch(_trackedDevicePoseMat4[0]));
}

glm::mat4 OpenVrDisplayPlugin::getEyeToHeadTransform(Eye eye) const {
    return _eyesData[eye]._eyeOffset;
}

glm::mat4 OpenVrDisplayPlugin::getHeadPose(uint32_t frameIndex) const {
    glm::mat4 result;
    {
        Lock lock(_mutex);
        result = _trackedDevicePoseMat4[0];

    }
    return result;
}


void OpenVrDisplayPlugin::submitSceneTexture(uint32_t frameIndex, uint32_t sceneTexture, const glm::uvec2& sceneSize) {
    WindowOpenGLDisplayPlugin::submitSceneTexture(frameIndex, sceneTexture, sceneSize);
}

void OpenVrDisplayPlugin::internalPresent() {
    // Flip y-axis since GL UV coords are backwards.
    static vr::VRTextureBounds_t leftBounds{ 0, 0, 0.5f, 1 };
    static vr::VRTextureBounds_t rightBounds{ 0.5f, 0, 1, 1 };
    vr::Texture_t texture{ (void*)_currentSceneTexture, vr::API_OpenGL, vr::ColorSpace_Auto };
    {
        Lock lock(_mutex);
        _compositor->Submit(vr::Eye_Left, &texture, &leftBounds);
        _compositor->Submit(vr::Eye_Right, &texture, &rightBounds);
    }
    glFinish();
    {
        Lock lock(_mutex);
        _compositor->WaitGetPoses(_trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
        for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
            _trackedDevicePoseMat4[i] = _sensorResetMat * toGlm(_trackedDevicePose[i].mDeviceToAbsoluteTracking);
        }
        openvr_for_each_eye([&](vr::Hmd_Eye eye) {
            _eyesData[eye]._pose = _trackedDevicePoseMat4[0];
        });
    }

    //WindowOpenGLDisplayPlugin::internalPresent();
}
