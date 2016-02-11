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
static vr::TrackedDevicePose_t _presentThreadTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
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
            PerEyeData& eyeData = _eyesData[eye];
            eyeData._projectionMatrix = toGlm(_hmd->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, vr::API_OpenGL));
            eyeData._eyeOffset = toGlm(_hmd->GetEyeToHeadTransform(eye));
        });
    }

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

    enableVsync(false);
    // Only enable mirroring if we know vsync is disabled
    _enablePreview = !isVsyncEnabled();
}

uvec2 OpenVrDisplayPlugin::getRecommendedRenderSize() const {
    return _renderTargetSize;
}

mat4 OpenVrDisplayPlugin::getProjection(Eye eye, const mat4& baseProjection) const {
    // FIXME hack to ensure that we don't crash trying to get the combined matrix
    if (eye == Mono) {
        eye = Left;
    }
    Lock lock(_poseMutex);
    return _eyesData[eye]._projectionMatrix;
}

void OpenVrDisplayPlugin::resetSensors() {
    Lock lock(_poseMutex);
    glm::mat4 m = toGlm(_trackedDevicePose[0].mDeviceToAbsoluteTracking);
    _sensorResetMat = glm::inverse(cancelOutRollAndPitch(m));
}

glm::mat4 OpenVrDisplayPlugin::getEyeToHeadTransform(Eye eye) const {
    Lock lock(_poseMutex);
    return _eyesData[eye]._eyeOffset;
}

glm::mat4 OpenVrDisplayPlugin::getHeadPose(uint32_t frameIndex) const {
    Lock lock(_poseMutex);
    return _trackedDevicePoseMat4[0];
}


void OpenVrDisplayPlugin::submitSceneTexture(uint32_t frameIndex, uint32_t sceneTexture, const glm::uvec2& sceneSize) {
    WindowOpenGLDisplayPlugin::submitSceneTexture(frameIndex, sceneTexture, sceneSize);
}

void OpenVrDisplayPlugin::internalPresent() {
    // Flip y-axis since GL UV coords are backwards.
    static vr::VRTextureBounds_t leftBounds{ 0, 0, 0.5f, 1 };
    static vr::VRTextureBounds_t rightBounds{ 0.5f, 0, 1, 1 };

    // screen preview mirroring
    if (_enablePreview) {
        auto windowSize = toGlm(_window->size());
        if (_monoPreview) {
            glViewport(0, 0, windowSize.x * 2, windowSize.y);
            glScissor(0, windowSize.y, windowSize.x, windowSize.y);
        } else {
            glViewport(0, 0, windowSize.x, windowSize.y);
        }
        glBindTexture(GL_TEXTURE_2D, _currentSceneTexture);
        GLenum err = glGetError();
        Q_ASSERT(0 == err);
        drawUnitQuad();
    }

    vr::Texture_t texture{ (void*)_currentSceneTexture, vr::API_OpenGL, vr::ColorSpace_Auto };

    _compositor->Submit(vr::Eye_Left, &texture, &leftBounds);
    _compositor->Submit(vr::Eye_Right, &texture, &rightBounds);

    glFinish();

    if (_enablePreview) {
        swapBuffers();
    }

    _compositor->WaitGetPoses(_presentThreadTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

    {
        // copy and process _presentThreadTrackedDevicePoses
        Lock lock(_poseMutex);
        for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
            _trackedDevicePose[i] = _presentThreadTrackedDevicePose[i];
            _trackedDevicePoseMat4[i] = _sensorResetMat * toGlm(_trackedDevicePose[i].mDeviceToAbsoluteTracking);
        }
        openvr_for_each_eye([&](vr::Hmd_Eye eye) {
            _eyesData[eye]._pose = _trackedDevicePoseMat4[0];
        });
    }

    //WindowOpenGLDisplayPlugin::internalPresent();
}
