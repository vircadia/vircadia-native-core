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

#include <controllers/Pose.h>
#include <PerfStat.h>
#include <ui-plugins/PluginContainer.h>
#include <ViewFrustum.h>
#include <display-plugins/CompositorHelper.h>
#include <shared/NsightHelpers.h>
#include "OpenVrHelpers.h"

Q_DECLARE_LOGGING_CATEGORY(displayplugins)

const QString OpenVrDisplayPlugin::NAME("OpenVR (Vive)");
const QString StandingHMDSensorMode = "Standing HMD Sensor Mode"; // this probably shouldn't be hardcoded here

static vr::IVRCompositor* _compositor { nullptr };
vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];

mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];
vec3 _trackedDeviceLinearVelocities[vr::k_unMaxTrackedDeviceCount];
vec3 _trackedDeviceAngularVelocities[vr::k_unMaxTrackedDeviceCount];

static mat4 _sensorResetMat;
static std::array<vr::Hmd_Eye, 2> VR_EYES { { vr::Eye_Left, vr::Eye_Right } };
bool _openVrDisplayActive { false };

bool OpenVrDisplayPlugin::isSupported() const {
    return openVrSupported();
}

void OpenVrDisplayPlugin::init() {
    Plugin::init();

    _lastGoodHMDPose.m[0][0] = 1.0f;
    _lastGoodHMDPose.m[0][1] = 0.0f;
    _lastGoodHMDPose.m[0][2] = 0.0f;
    _lastGoodHMDPose.m[0][3] = 0.0f;
    _lastGoodHMDPose.m[1][0] = 0.0f;
    _lastGoodHMDPose.m[1][1] = 1.0f;
    _lastGoodHMDPose.m[1][2] = 0.0f;
    _lastGoodHMDPose.m[1][3] = 1.8f;
    _lastGoodHMDPose.m[2][0] = 0.0f;
    _lastGoodHMDPose.m[2][1] = 0.0f;
    _lastGoodHMDPose.m[2][2] = 1.0f;
    _lastGoodHMDPose.m[2][3] = 0.0f;

    emit deviceConnected(getName());
}

bool OpenVrDisplayPlugin::internalActivate() {
    _openVrDisplayActive = true;
    _container->setIsOptionChecked(StandingHMDSensorMode, true);

    if (!_system) {
        _system = acquireOpenVrSystem();
    }
    if (!_system) {
        qWarning() << "Failed to initialize OpenVR";
        return false;
    }

    _system->GetRecommendedRenderTargetSize(&_renderTargetSize.x, &_renderTargetSize.y);
    // Recommended render target size is per-eye, so double the X size for 
    // left + right eyes
    _renderTargetSize.x *= 2;

    withRenderThreadLock([&] {
        openvr_for_each_eye([&](vr::Hmd_Eye eye) {
            _eyeOffsets[eye] = toGlm(_system->GetEyeToHeadTransform(eye));
            _eyeProjections[eye] = toGlm(_system->GetProjectionMatrix(eye, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP, vr::API_OpenGL));
        });
        // FIXME Calculate the proper combined projection by using GetProjectionRaw values from both eyes
        _cullingProjection = _eyeProjections[0];
    });

    _compositor = vr::VRCompositor();
    Q_ASSERT(_compositor);

    // enable async time warp
    // _compositor->ForceInterleavedReprojectionOn(true);

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
        #if DEV_BUILD
            qDebug() << "OpenVR: error could not get chaperone pointer";
        #endif
    }

    return Parent::internalActivate();
}

void OpenVrDisplayPlugin::internalDeactivate() {
    Parent::internalDeactivate();
    _openVrDisplayActive = false;
    _container->setIsOptionChecked(StandingHMDSensorMode, false);
    if (_system) {
        // Invalidate poses. It's fine if someone else sets these shared values, but we're about to stop updating them, and
        // we don't want ViveControllerManager to consider old values to be valid.
        for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
            _trackedDevicePose[i].bPoseIsValid = false;
        }
        releaseOpenVrSystem();
        _system = nullptr;
    }
    _compositor = nullptr;
}

void OpenVrDisplayPlugin::customizeContext() {
    // Display plugins in DLLs must initialize glew locally
    static std::once_flag once;
    std::call_once(once, [] {
        glewExperimental = true;
        GLenum err = glewInit();
        glGetError(); // clear the potential error from glewExperimental
    });

    Parent::customizeContext();
}

void OpenVrDisplayPlugin::resetSensors() {
    withRenderThreadLock([&] {
        glm::mat4 m = toGlm(_trackedDevicePose[0].mDeviceToAbsoluteTracking);
        _sensorResetMat = glm::inverse(cancelOutRollAndPitch(m));
    });
}

static bool isBadPose(vr::HmdMatrix34_t* mat) {
    if (mat->m[1][3] < -0.2f) {
        return true;
    }
    return false;
}

bool OpenVrDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    handleOpenVrEvents();

    double displayFrequency = _system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
    double frameDuration = 1.f / displayFrequency;
    double vsyncToPhotons = _system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);

    _currentRenderFrameInfo = FrameInfo();
#if THREADED_PRESENT
    // 3 frames of prediction + vsyncToPhotons = 44ms total
    const double NUM_PREDICTION_FRAMES = 3.0f;
    _currentRenderFrameInfo.predictedDisplayTime = NUM_PREDICTION_FRAMES * frameDuration + vsyncToPhotons;
#else
    _currentRenderFrameInfo.predictedDisplayTime = frameDuration + vsyncToPhotons;
#endif

    _system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, _currentRenderFrameInfo.predictedDisplayTime, _trackedDevicePose, vr::k_unMaxTrackedDeviceCount);

    // HACK: when interface is launched and steam vr is NOT running, openvr will return bad HMD poses for a few frames
    // To workaround this, filter out any hmd poses that are obviously bad, i.e. beneath the floor.
    if (isBadPose(&_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking)) {
        qDebug() << "WARNING: ignoring bad hmd pose from openvr";

        // use the last known good HMD pose
        _trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking = _lastGoodHMDPose;
    } else {
        _lastGoodHMDPose = _trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
    }

    vr::TrackedDeviceIndex_t handIndices[2] { vr::k_unTrackedDeviceIndexInvalid, vr::k_unTrackedDeviceIndexInvalid };
    {
        vr::TrackedDeviceIndex_t controllerIndices[2] ;
        auto trackedCount = _system->GetSortedTrackedDeviceIndicesOfClass(vr::TrackedDeviceClass_Controller, controllerIndices, 2);
        // Find the left and right hand controllers, if they exist
        for (uint32_t i = 0; i < std::min<uint32_t>(trackedCount, 2); ++i) {
            if (_trackedDevicePose[i].bPoseIsValid) {
                auto role = _system->GetControllerRoleForTrackedDeviceIndex(controllerIndices[i]);
                if (vr::TrackedControllerRole_LeftHand == role) {
                    handIndices[0] = controllerIndices[i];
                } else if (vr::TrackedControllerRole_RightHand == role) {
                    handIndices[1] = controllerIndices[i];
                }
            }
        }
    }

    // copy and process predictedTrackedDevicePoses
    for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
        _trackedDevicePoseMat4[i] = _sensorResetMat * toGlm(_trackedDevicePose[i].mDeviceToAbsoluteTracking);
        _trackedDeviceLinearVelocities[i] = transformVectorFast(_sensorResetMat, toGlm(_trackedDevicePose[i].vVelocity));
        _trackedDeviceAngularVelocities[i] = transformVectorFast(_sensorResetMat, toGlm(_trackedDevicePose[i].vAngularVelocity));
    }
    _currentRenderFrameInfo.rawRenderPose = toGlm(_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
    _currentRenderFrameInfo.renderPose = _trackedDevicePoseMat4[vr::k_unTrackedDeviceIndex_Hmd];

    bool keyboardVisible = isOpenVrKeyboardShown();

    std::array<mat4, 2> handPoses;
    if (!keyboardVisible) {
        for (int i = 0; i < 2; ++i) {
            if (handIndices[i] == vr::k_unTrackedDeviceIndexInvalid) {
                continue;
            }
            auto deviceIndex = handIndices[i];
            const mat4& mat = _trackedDevicePoseMat4[deviceIndex];
            const vec3& linearVelocity = _trackedDeviceLinearVelocities[deviceIndex];
            const vec3& angularVelocity = _trackedDeviceAngularVelocities[deviceIndex];
            auto correctedPose = openVrControllerPoseToHandPose(i == 0, mat, linearVelocity, angularVelocity);
            static const glm::quat HAND_TO_LASER_ROTATION = glm::rotation(Vectors::UNIT_Z, Vectors::UNIT_NEG_Y);
            handPoses[i] = glm::translate(glm::mat4(), correctedPose.translation) * glm::mat4_cast(correctedPose.rotation * HAND_TO_LASER_ROTATION);
        }
    }

    withRenderThreadLock([&] {
        _uiModelTransform = DependencyManager::get<CompositorHelper>()->getModelTransform();
        // Make controller poses available to the presentation thread
        _handPoses = handPoses;
        _frameInfos[frameIndex] = _currentRenderFrameInfo;
    });
    return Parent::beginFrameRender(frameIndex);
}

void OpenVrDisplayPlugin::hmdPresent() {
    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)_currentPresentFrameIndex)

    // Flip y-axis since GL UV coords are backwards.
    static vr::VRTextureBounds_t leftBounds { 0, 0, 0.5f, 1 };
    static vr::VRTextureBounds_t rightBounds { 0.5f, 0, 1, 1 };

    vr::Texture_t texture { (void*)oglplus::GetName(_compositeFramebuffer->color), vr::API_OpenGL, vr::ColorSpace_Auto };

    _compositor->Submit(vr::Eye_Left, &texture, &leftBounds);
    _compositor->Submit(vr::Eye_Right, &texture, &rightBounds);
}

void OpenVrDisplayPlugin::postPreview() {
    PROFILE_RANGE_EX(__FUNCTION__, 0xff00ff00, (uint64_t)_currentPresentFrameIndex)

    vr::TrackedDevicePose_t currentTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    _compositor->WaitGetPoses(currentTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
    _hmdActivityLevel = _system->GetTrackedDeviceActivityLevel(vr::k_unTrackedDeviceIndex_Hmd);
}

bool OpenVrDisplayPlugin::isHmdMounted() const {
    return _hmdActivityLevel == vr::k_EDeviceActivityLevel_UserInteraction;
}

void OpenVrDisplayPlugin::updatePresentPose() {
    mat4 sensorResetMat;
    withPresentThreadLock([&] {
        sensorResetMat = _sensorResetMat;
    });
    {
        float fSecondsSinceLastVsync;
        _system->GetTimeSinceLastVsync(&fSecondsSinceLastVsync, nullptr);
        float fDisplayFrequency = _system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
        float fFrameDuration = 1.f / fDisplayFrequency;
        float fVsyncToPhotons = _system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);
        float fPredictedSecondsFromNow = fFrameDuration - fSecondsSinceLastVsync + fVsyncToPhotons;
        vr::TrackedDevicePose_t pose;
        _system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, fPredictedSecondsFromNow, &pose, 1);
        _currentPresentFrameInfo.rawPresentPose = toGlm(pose.mDeviceToAbsoluteTracking);
    }
    _currentPresentFrameInfo.presentPose = sensorResetMat * _currentPresentFrameInfo.rawPresentPose;
    mat3 renderRotation(_currentPresentFrameInfo.rawRenderPose);
    mat3 presentRotation(_currentPresentFrameInfo.rawPresentPose);
    _currentPresentFrameInfo.presentReprojection = glm::mat3(glm::inverse(renderRotation) * presentRotation);
}

bool OpenVrDisplayPlugin::suppressKeyboard() { 
    if (isOpenVrKeyboardShown()) {
        return false;
    }
    if (!_keyboardSupressionCount.fetch_add(1)) {
        disableOpenVrKeyboard();
    }
    return true;
}

void OpenVrDisplayPlugin::unsuppressKeyboard() {
    if (_keyboardSupressionCount == 0) {
        qWarning() << "Attempted to unsuppress a keyboard that was not suppressed";
        return;
    }
    if (1 == _keyboardSupressionCount.fetch_sub(1)) {
        enableOpenVrKeyboard(_container);
    }
}

bool OpenVrDisplayPlugin::isKeyboardVisible() {
    return isOpenVrKeyboardShown(); 
}
