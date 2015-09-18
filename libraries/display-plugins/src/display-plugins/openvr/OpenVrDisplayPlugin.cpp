//
//  Created by Bradley Austin Davis on 2015/05/12
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenVrDisplayPlugin.h"

#if defined(Q_OS_WIN)

#include <memory>

#include <QMainWindow>
#include <QGLWidget>
#include <GLMHelpers.h>
#include <GlWindow.h>
#include <QEvent>
#include <QResizeEvent>

#include <PerfStat.h>
#include <plugins/PluginContainer.h>
#include <ViewFrustum.h>

#include "OpenVrHelpers.h"
#include "GLMHelpers.h"

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(displayplugins)
Q_LOGGING_CATEGORY(displayplugins, "hifi.displayplugins")

const QString OpenVrDisplayPlugin::NAME("OpenVR (Vive)");

const QString StandingHMDSensorMode = "Standing HMD Sensor Mode"; // this probably shouldn't be hardcoded here

const QString & OpenVrDisplayPlugin::getName() const {
    return NAME;
}

vr::IVRSystem* _hmd{ nullptr };
int hmdRefCount = 0;
static vr::IVRCompositor* _compositor{ nullptr };
vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];
static mat4 _sensorResetMat;
static uvec2 _windowSize;
static ivec2 _windowPosition;
static uvec2 _renderTargetSize;

struct PerEyeData {
    uvec2 _viewportOrigin;
    uvec2 _viewportSize;
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
    bool success = vr::VR_IsHmdPresent();
    if (success) {
        vr::HmdError eError = vr::HmdError_None;
        auto hmd = vr::VR_Init(&eError);
        success = (hmd != nullptr);
        vr::VR_Shutdown();
    }
    return success;
}

void OpenVrDisplayPlugin::activate() {
    CONTAINER->setIsOptionChecked(StandingHMDSensorMode, true);

    hmdRefCount++;
    vr::HmdError eError = vr::HmdError_None;
    if (!_hmd) {
        _hmd = vr::VR_Init(&eError);
        Q_ASSERT(eError == vr::HmdError_None);
    }
    Q_ASSERT(_hmd);

    _hmd->GetWindowBounds(&_windowPosition.x, &_windowPosition.y, &_windowSize.x, &_windowSize.y);
    _hmd->GetRecommendedRenderTargetSize(&_renderTargetSize.x, &_renderTargetSize.y);
    // Recommended render target size is per-eye, so double the X size for 
    // left + right eyes
    _renderTargetSize.x *= 2;
    openvr_for_each_eye([&](vr::Hmd_Eye eye) {
        PerEyeData& eyeData = _eyesData[eye];
        _hmd->GetEyeOutputViewport(eye, 
            &eyeData._viewportOrigin.x, &eyeData._viewportOrigin.y, 
            &eyeData._viewportSize.x, &eyeData._viewportSize.y);
        eyeData._projectionMatrix = toGlm(_hmd->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, vr::API_OpenGL));
        eyeData._eyeOffset = toGlm(_hmd->GetEyeToHeadTransform(eye));
    });


    _compositor = (vr::IVRCompositor*)vr::VR_GetGenericInterface(vr::IVRCompositor_Version, &eError);
    Q_ASSERT(eError == vr::HmdError_None);
    Q_ASSERT(_compositor);

    _compositor->SetGraphicsDevice(vr::Compositor_DeviceType_OpenGL, NULL);

    uint32_t unSize = _compositor->GetLastError(NULL, 0);
    if (unSize > 1) {
        char* buffer = new char[unSize];
        _compositor->GetLastError(buffer, unSize);
        printf("Compositor - %s\n", buffer);
        delete[] buffer;
    }
    Q_ASSERT(unSize <= 1);
    WindowOpenGLDisplayPlugin::activate();
}

void OpenVrDisplayPlugin::deactivate() {
    CONTAINER->setIsOptionChecked(StandingHMDSensorMode, false);

    hmdRefCount--;

    if (hmdRefCount == 0 && _hmd) {
        vr::VR_Shutdown();
        _hmd = nullptr;
    }
    _compositor = nullptr;
    WindowOpenGLDisplayPlugin::deactivate();
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

glm::mat4 OpenVrDisplayPlugin::getEyePose(Eye eye) const {
    return getHeadPose() * _eyesData[eye]._eyeOffset;
}

glm::mat4 OpenVrDisplayPlugin::getHeadPose() const {
    return _trackedDevicePoseMat4[0];
}

void OpenVrDisplayPlugin::customizeContext() {
    WindowOpenGLDisplayPlugin::customizeContext();
}

void OpenVrDisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    // Flip y-axis since GL UV coords are backwards.
    static vr::Compositor_TextureBounds leftBounds{ 0, 1, 0.5f, 0 };
    static vr::Compositor_TextureBounds rightBounds{ 0.5f, 1, 1, 0 };
    _compositor->Submit(vr::Eye_Left, (void*)finalTexture, &leftBounds);
    _compositor->Submit(vr::Eye_Right, (void*)finalTexture, &rightBounds);
    glFinish();
}

void OpenVrDisplayPlugin::finishFrame() {
//    swapBuffers();
    doneCurrent();
    _compositor->WaitGetPoses(_trackedDevicePose, vr::k_unMaxTrackedDeviceCount);
    for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
        _trackedDevicePoseMat4[i] = _sensorResetMat * toGlm(_trackedDevicePose[i].mDeviceToAbsoluteTracking);
    }
    openvr_for_each_eye([&](vr::Hmd_Eye eye) {
        _eyesData[eye]._pose = _trackedDevicePoseMat4[0];
    });
};

#endif

