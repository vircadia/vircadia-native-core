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
vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];
vec3 _trackedDeviceLinearVelocities[vr::k_unMaxTrackedDeviceCount];
vec3 _trackedDeviceAngularVelocities[vr::k_unMaxTrackedDeviceCount];
static mat4 _sensorResetMat;
static std::array<vr::Hmd_Eye, 2> VR_EYES { { vr::Eye_Left, vr::Eye_Right } };

bool OpenVrDisplayPlugin::isSupported() const {
    return vr::VR_IsHmdPresent();
}

void OpenVrDisplayPlugin::activate() {
    _container->setIsOptionChecked(StandingHMDSensorMode, true);

    if (!_system) {
        _system = acquireOpenVrSystem();
    }
    Q_ASSERT(_system);

    _system->GetRecommendedRenderTargetSize(&_renderTargetSize.x, &_renderTargetSize.y);
    // Recommended render target size is per-eye, so double the X size for 
    // left + right eyes
    _renderTargetSize.x *= 2;

    {
        Lock lock(_poseMutex);
        openvr_for_each_eye([&](vr::Hmd_Eye eye) {
            _eyeOffsets[eye] = toGlm(_system->GetEyeToHeadTransform(eye));
            _eyeProjections[eye] = toGlm(_system->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, vr::API_OpenGL));
        });
        // FIXME Calculate the proper combined projection by using GetProjectionRaw values from both eyes
        _cullingProjection = _eyeProjections[0];

    }

    _compositor = vr::VRCompositor();
    Q_ASSERT(_compositor);
    HmdDisplayPlugin::activate();

    // set up default sensor space such that the UI overlay will align with the front of the room.
    auto chaperone = vr::VRChaperone();
    if (chaperone) {
        float const UI_RADIUS = 1.0f;
        float const UI_HEIGHT = 1.6f;
        float const UI_Z_OFFSET = 0.5;

        float xSize, zSize;
        chaperone->GetPlayAreaSize(&xSize, &zSize);
        glm::vec3 uiPos(0.0f, UI_HEIGHT, UI_RADIUS - (0.5f * zSize) - UI_Z_OFFSET);
        _sensorResetMat = glm::inverse(createMatFromQuatAndPos(glm::quat(), uiPos));
    } else {
        qDebug() << "OpenVR: error could not get chaperone pointer";
    }
}

void OpenVrDisplayPlugin::deactivate() {
    // Base class deactivate must come before our local deactivate
    // because the OpenGL base class handles the wait for the present 
    // thread before continuing
    HmdDisplayPlugin::deactivate();
    _container->setIsOptionChecked(StandingHMDSensorMode, false);
    if (_system) {
        releaseOpenVrSystem();
        _system = nullptr;
    }
    _compositor = nullptr;
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

    float displayFrequency = _system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
    float frameDuration = 1.f / displayFrequency;
    float vsyncToPhotons = _system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);

#if THREADED_PRESENT
    // TODO: this seems awfuly long, 44ms total, but it produced the best results.
    const float NUM_PREDICTION_FRAMES = 3.0f;
    float predictedSecondsFromNow = NUM_PREDICTION_FRAMES * frameDuration + vsyncToPhotons;
#else
    uint64_t frameCounter;
    float timeSinceLastVsync;
    _system->GetTimeSinceLastVsync(&timeSinceLastVsync, &frameCounter);
    float predictedSecondsFromNow = 3.0f * frameDuration - timeSinceLastVsync + vsyncToPhotons;
#endif

    vr::TrackedDevicePose_t predictedTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    _system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, predictedSecondsFromNow, predictedTrackedDevicePose, vr::k_unMaxTrackedDeviceCount);

    // copy and process predictedTrackedDevicePoses
    for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
        _trackedDevicePose[i] = predictedTrackedDevicePose[i];
        _trackedDevicePoseMat4[i] = _sensorResetMat * toGlm(_trackedDevicePose[i].mDeviceToAbsoluteTracking);
        _trackedDeviceLinearVelocities[i] = transformVectorFast(_sensorResetMat, toGlm(_trackedDevicePose[i].vVelocity));
        _trackedDeviceAngularVelocities[i] = transformVectorFast(_sensorResetMat, toGlm(_trackedDevicePose[i].vAngularVelocity));
    }
    return _trackedDevicePoseMat4[0];
}

void OpenVrDisplayPlugin::hmdPresent() {
    // Flip y-axis since GL UV coords are backwards.
    static vr::VRTextureBounds_t leftBounds{ 0, 0, 0.5f, 1 };
    static vr::VRTextureBounds_t rightBounds{ 0.5f, 0, 1, 1 };
    
    vr::Texture_t texture { (void*)oglplus::GetName(_compositeFramebuffer->color), vr::API_OpenGL, vr::ColorSpace_Auto };

    _compositor->Submit(vr::Eye_Left, &texture, &leftBounds);
    _compositor->Submit(vr::Eye_Right, &texture, &rightBounds);

    vr::TrackedDevicePose_t currentTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    _compositor->WaitGetPoses(currentTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
    _hmdActivityLevel = _system->GetTrackedDeviceActivityLevel(vr::k_unTrackedDeviceIndex_Hmd);
}

bool OpenVrDisplayPlugin::isHmdMounted() const {
    return _hmdActivityLevel == vr::k_EDeviceActivityLevel_UserInteraction;
}

