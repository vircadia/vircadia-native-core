//
//  ViveControllerManager.cpp
//
//  Created by Sam Gondelman on 6/29/15.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ViveControllerManager.h"
#include <algorithm>
#include <string>

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4091 )
#pragma warning( disable : 4334 )
#endif

#ifdef VIVE_PRO_EYE
#include <SRanipal.h>
#include <SRanipal_Eye.h>
#include <SRanipal_Enums.h>
#include <interface_gesture.hpp>
#endif

#ifdef _WIN32
#pragma warning( pop )
#endif

#include <PerfStat.h>
#include <PathUtils.h>
#include <GeometryCache.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <DeferredLightingEffect.h>
#include <NumericalConstants.h>
#include <ui-plugins/PluginContainer.h>
#include <UserActivityLogger.h>
#include <NumericalConstants.h>
#include <Preferences.h>
#include <SettingHandle.h>
#include <OffscreenUi.h>
#include <GLMHelpers.h>
#include <AvatarConstants.h>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>
#include <ui-plugins/PluginContainer.h>
#include <plugins/DisplayPlugin.h>
#include <ThreadHelpers.h>

#include <controllers/UserInputMapper.h>
#include <plugins/InputConfiguration.h>
#include <controllers/StandardControls.h>

#include "OpenVrDisplayPlugin.h"

extern PoseData _nextSimPoseData;

vr::IVRSystem* acquireOpenVrSystem();
void releaseOpenVrSystem();

static const QString OPENVR_LAYOUT = QString("OpenVrConfiguration.qml");
const quint64 CALIBRATION_TIMELAPSE = 1 * USECS_PER_SECOND;

static const int MIN_HEAD = 1;
static const int MIN_PUCK_COUNT = 2;
static const int MIN_FEET_AND_HIPS = 3;
static const int MIN_FEET_HIPS_CHEST = 4;
static const int MIN_FEET_HIPS_SHOULDERS = 5;
static const int MIN_FEET_HIPS_CHEST_SHOULDERS = 6;

static const int FIRST_FOOT = 0;
static const int SECOND_FOOT = 1;
static const int HIP = 2;
static const int CHEST = 3;

#ifdef VIVE_PRO_EYE
enum ViveHandJointIndex {
    HAND = 0,
    THUMB_1,
    THUMB_2,
    THUMB_3,
    THUMB_4,
    INDEX_1,
    INDEX_2,
    INDEX_3,
    INDEX_4,
    MIDDLE_1,
    MIDDLE_2,
    MIDDLE_3,
    MIDDLE_4,
    RING_1,
    RING_2,
    RING_3,
    RING_4,
    PINKY_1,
    PINKY_2,
    PINKY_3,
    PINKY_4,

    Size
};
#endif

const char* ViveControllerManager::NAME { "OpenVR" };

const std::map<vr::ETrackingResult, QString> TRACKING_RESULT_TO_STRING = {
    {vr::TrackingResult_Uninitialized, QString("vr::TrackingResult_Uninitialized")},
    {vr::TrackingResult_Calibrating_InProgress, QString("vr::TrackingResult_Calibrating_InProgess")},
    {vr::TrackingResult_Calibrating_OutOfRange, QString("TrackingResult_Calibrating_OutOfRange")},
    {vr::TrackingResult_Running_OK, QString("TrackingResult_Running_Ok")},
    {vr::TrackingResult_Running_OutOfRange, QString("TrackingResult_Running_OutOfRange")}
};

static glm::mat4 computeOffset(glm::mat4 defaultToReferenceMat, glm::mat4 defaultJointMat, controller::Pose puckPose) {
    glm::mat4 poseMat = createMatFromQuatAndPos(puckPose.rotation, puckPose.translation);
    glm::mat4 referenceJointMat = defaultToReferenceMat * defaultJointMat;
    return glm::inverse(poseMat) * referenceJointMat;
}

static bool sortPucksYPosition(const PuckPosePair& firstPuck, const PuckPosePair& secondPuck) {
    return (firstPuck.second.translation.y < secondPuck.second.translation.y);
}

static bool determineLimbOrdering(const controller::Pose& poseA, const controller::Pose& poseB, glm::vec3 axis, glm::vec3 axisOrigin) {
    glm::vec3 poseAPosition = poseA.getTranslation();
    glm::vec3 poseBPosition = poseB.getTranslation();

    glm::vec3 poseAVector = poseAPosition - axisOrigin;
    glm::vec3 poseBVector = poseBPosition - axisOrigin;

    float poseAProjection = glm::dot(poseAVector, axis);
    float poseBProjection = glm::dot(poseBVector, axis);
    return (poseAProjection > poseBProjection);
}

static glm::vec3 getReferenceHeadXAxis(glm::mat4 defaultToReferenceMat, glm::mat4 defaultHead) {
    glm::mat4 finalHead = defaultToReferenceMat * defaultHead;
    return glmExtractRotation(finalHead) * Vectors::UNIT_X;
}

static glm::vec3 getReferenceHeadPosition(glm::mat4 defaultToReferenceMat, glm::mat4 defaultHead) {
    glm::mat4 finalHead = defaultToReferenceMat * defaultHead;
    return extractTranslation(finalHead);
}

static QString deviceTrackingResultToString(vr::ETrackingResult trackingResult) {
    QString result;
    auto iterator = TRACKING_RESULT_TO_STRING.find(trackingResult);

    if (iterator != TRACKING_RESULT_TO_STRING.end()) {
        return iterator->second;
    }
    return result;
}

#ifdef VIVE_PRO_EYE
class ViveProEyeReadThread : public QThread {
public:
    ViveProEyeReadThread() {
        setObjectName("OpenVR ViveProEye Read Thread");
    }
    void run() override {
        while (!quit) {
            ViveSR::anipal::Eye::EyeData eyeData;
            int result = ViveSR::anipal::Eye::GetEyeData(&eyeData);
            {
                QMutexLocker locker(&eyeDataMutex);
                eyeDataBuffer.getEyeDataResult = result;
                if (result == ViveSR::Error::WORK) {
                    uint64_t leftValids = eyeData.verbose_data.left.eye_data_validata_bit_mask;
                    uint64_t rightValids = eyeData.verbose_data.right.eye_data_validata_bit_mask;

                    eyeDataBuffer.leftDirectionValid =
                        (leftValids & (uint64_t)ViveSR::anipal::Eye::SINGLE_EYE_DATA_GAZE_DIRECTION_VALIDITY) > (uint64_t)0;
                    eyeDataBuffer.rightDirectionValid =
                        (rightValids & (uint64_t)ViveSR::anipal::Eye::SINGLE_EYE_DATA_GAZE_DIRECTION_VALIDITY) > (uint64_t)0;
                    eyeDataBuffer.leftOpennessValid =
                        (leftValids & (uint64_t)ViveSR::anipal::Eye::SINGLE_EYE_DATA_EYE_OPENNESS_VALIDITY) > (uint64_t)0;
                    eyeDataBuffer.rightOpennessValid =
                        (rightValids & (uint64_t)ViveSR::anipal::Eye::SINGLE_EYE_DATA_EYE_OPENNESS_VALIDITY) > (uint64_t)0;

                    float *leftGaze = eyeData.verbose_data.left.gaze_direction_normalized.elem_;
                    float *rightGaze = eyeData.verbose_data.right.gaze_direction_normalized.elem_;
                    eyeDataBuffer.leftEyeGaze = glm::vec3(leftGaze[0], leftGaze[1], leftGaze[2]);
                    eyeDataBuffer.rightEyeGaze = glm::vec3(rightGaze[0], rightGaze[1], rightGaze[2]);

                    eyeDataBuffer.leftEyeOpenness = eyeData.verbose_data.left.eye_openness;
                    eyeDataBuffer.rightEyeOpenness = eyeData.verbose_data.right.eye_openness;
                }
            }
        }
    }

    bool quit { false };

    // mutex and buffer for moving data from this thread to the other one
    QMutex eyeDataMutex;
    EyeDataBuffer eyeDataBuffer;
};
#endif


static QString outOfRangeDataStrategyToString(ViveControllerManager::OutOfRangeDataStrategy strategy) {
    switch (strategy) {
    default:
    case ViveControllerManager::OutOfRangeDataStrategy::None:
        return "None";
    case ViveControllerManager::OutOfRangeDataStrategy::Freeze:
        return "Freeze";
    case ViveControllerManager::OutOfRangeDataStrategy::Drop:
        return "Drop";
    case ViveControllerManager::OutOfRangeDataStrategy::DropAfterDelay:
        return "DropAfterDelay";
    }
}

static ViveControllerManager::OutOfRangeDataStrategy stringToOutOfRangeDataStrategy(const QString& string) {
    if (string == "Drop") {
        return ViveControllerManager::OutOfRangeDataStrategy::Drop;
    } else if (string == "Freeze") {
        return ViveControllerManager::OutOfRangeDataStrategy::Freeze;
    } else if (string == "DropAfterDelay") {
        return ViveControllerManager::OutOfRangeDataStrategy::DropAfterDelay;
    } else {
        return ViveControllerManager::OutOfRangeDataStrategy::None;
    }
}

bool ViveControllerManager::isDesktopMode() {
    if (_container) {
        return !_container->getActiveDisplayPlugin()->isHmd();
    }
    return false;
}

void ViveControllerManager::calibrate() {
    if (isSupported()) {
        _inputDevice->calibrateNextFrame();
    }
}

bool ViveControllerManager::uncalibrate() {
    if (isSupported()) {
        _inputDevice->uncalibrate();
        return true;
    }
    return false;
}

bool ViveControllerManager::isSupported() const {
    return openVrSupported();
}

void ViveControllerManager::setConfigurationSettings(const QJsonObject configurationSettings) {
    if (isSupported()) {
        if (configurationSettings.contains("desktopMode")) {
            _desktopMode = configurationSettings["desktopMode"].toBool();
        }

        if (configurationSettings.contains("hmdDesktopTracking")) {
            _hmdDesktopTracking = configurationSettings["hmdDesktopTracking"].toBool();
        }

        if (configurationSettings.contains("eyeTrackingEnabled")) {
            _eyeTrackingEnabled = configurationSettings["eyeTrackingEnabled"].toBool();
        }

        _inputDevice->configureCalibrationSettings(configurationSettings);
        saveSettings();
    }
}

QJsonObject ViveControllerManager::configurationSettings() {
    if (isSupported()) {
        QJsonObject configurationSettings = _inputDevice->configurationSettings();
        configurationSettings["desktopMode"] = _desktopMode;
        configurationSettings["hmdDesktopTracking"] = _hmdDesktopTracking;
        configurationSettings["eyeTrackingEnabled"] = _eyeTrackingEnabled;
        return configurationSettings;
    }

    return QJsonObject();
}

QString ViveControllerManager::configurationLayout() {
    return OPENVR_LAYOUT;
}

bool isDeviceIndexActive(vr::IVRSystem*& system, uint32_t deviceIndex) {
    if (!system) {
        return false;
    }
    if (deviceIndex != vr::k_unTrackedDeviceIndexInvalid &&
        system->GetTrackedDeviceClass(deviceIndex) == vr::TrackedDeviceClass_Controller &&
        system->IsTrackedDeviceConnected(deviceIndex)) {
        vr::EDeviceActivityLevel activityLevel = system->GetTrackedDeviceActivityLevel(deviceIndex);
        if (activityLevel == vr::k_EDeviceActivityLevel_UserInteraction) {
            return true;
        }
    }
    return false;
}

bool isHandControllerActive(vr::IVRSystem*& system, vr::ETrackedControllerRole deviceRole) {
    if (!system) {
        return false;
    }
    auto deviceIndex = system->GetTrackedDeviceIndexForControllerRole(deviceRole);
    return isDeviceIndexActive(system, deviceIndex);
}

bool areBothHandControllersActive(vr::IVRSystem*& system) {
    return
        isHandControllerActive(system, vr::TrackedControllerRole_LeftHand) &&
        isHandControllerActive(system, vr::TrackedControllerRole_RightHand);
}

#ifdef VIVE_PRO_EYE
void ViveControllerManager::enableGestureDetection() {
    if (_viveCameraHandTracker) {
        return;
    }
    if (!ViveSR::anipal::Eye::IsViveProEye()) {
        return;
    }

// #define HAND_TRACKER_USE_EXTERNAL_TRANSFORM 1

#ifdef HAND_TRACKER_USE_EXTERNAL_TRANSFORM
    UseExternalTransform(true); // camera hand tracker results are in HMD frame
#else
    UseExternalTransform(false); // camera hand tracker results are in sensor frame
#endif
    GestureOption options; // defaults are GestureBackendAuto and GestureModeSkeleton
    GestureFailure gestureFailure = StartGestureDetection(&options);
    switch (gestureFailure) {
        case GestureFailureNone:
            qDebug() << "StartGestureDetection success";
            _viveCameraHandTracker = true;
            break;
        case GestureFailureOpenCL:
            qDebug() << "StartGestureDetection (Only on Windows) OpenCL is not supported on the machine";
            break;
        case GestureFailureCamera:
            qDebug() << "StartGestureDetection Start camera failed";
            break;
        case GestureFailureInternal:
            qDebug() << "StartGestureDetection Internal errors";
            break;
        case GestureFailureCPUOnPC:
            qDebug() << "StartGestureDetection CPU backend is not supported on Windows";
            break;
    }
}

void ViveControllerManager::disableGestureDetection() {
    if (!_viveCameraHandTracker) {
        return;
    }
    StopGestureDetection();
    _viveCameraHandTracker = false;
}
#endif

bool ViveControllerManager::activate() {
    InputPlugin::activate();

    loadSettings();

    if (!_system) {
        _system = acquireOpenVrSystem();
    }

    if (!_system) {
        return false;
    }

    enableOpenVrKeyboard(_container);

    // register with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(_inputDevice);
    _registeredWithInputMapper = true;

#ifdef VIVE_PRO_EYE
    if (ViveSR::anipal::Eye::IsViveProEye()) {
        qDebug() << "Vive Pro eye-tracking detected";

        int error = ViveSR::anipal::Initial(ViveSR::anipal::Eye::ANIPAL_TYPE_EYE, NULL);
        if (error == ViveSR::Error::WORK) {
            _viveProEye = true;
            qDebug() << "Successfully initialize Eye engine.";
        } else if (error == ViveSR::Error::RUNTIME_NOT_FOUND) {
            _viveProEye = false;
            qDebug() << "please follows SRanipal SDK guide to install SR_Runtime first";
        } else {
            _viveProEye = false;
            qDebug() << "Failed to initialize Eye engine. please refer to ViveSR error code:" << error;
        }

        if (_viveProEye) {
            _viveProEyeReadThread = std::make_shared<ViveProEyeReadThread>();
            connect(_viveProEyeReadThread.get(), &QThread::started, [] { setThreadName("ViveProEyeReadThread"); });
            _viveProEyeReadThread->start(QThread::HighPriority);
        }
    }
#endif

    return true;
}

void ViveControllerManager::deactivate() {
    InputPlugin::deactivate();

    disableOpenVrKeyboard();

    if (_system) {
        _container->makeRenderingContextCurrent();
        releaseOpenVrSystem();
        _system = nullptr;
    }

    _inputDevice->_poseStateMap.clear();

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->removeDevice(_inputDevice->_deviceID);
    _registeredWithInputMapper = false;

#ifdef VIVE_PRO_EYE
    if (_viveProEyeReadThread) {
        _viveProEyeReadThread->quit = true;
        _viveProEyeReadThread->wait();
        _viveProEyeReadThread = nullptr;
        ViveSR::anipal::Release(ViveSR::anipal::Eye::ANIPAL_TYPE_EYE);
    }
#endif

    saveSettings();
}

bool ViveControllerManager::isHeadControllerMounted() const {
    if (_inputDevice && _inputDevice->isHeadControllerMounted()) {
        return true;
    }
    vr::EDeviceActivityLevel activityLevel = _system->GetTrackedDeviceActivityLevel(vr::k_unTrackedDeviceIndex_Hmd);
    return activityLevel == vr::k_EDeviceActivityLevel_UserInteraction;
}

#ifdef VIVE_PRO_EYE
void ViveControllerManager::invalidateEyeInputs() {
    _inputDevice->_poseStateMap[controller::LEFT_EYE].valid = false;
    _inputDevice->_poseStateMap[controller::RIGHT_EYE].valid = false;
    _inputDevice->_axisStateMap[controller::EYEBLINK_L].valid = false;
    _inputDevice->_axisStateMap[controller::EYEBLINK_R].valid = false;
}

void ViveControllerManager::updateEyeTracker(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    if (!isHeadControllerMounted()) {
        invalidateEyeInputs();
        return;
    }

    EyeDataBuffer eyeDataBuffer;
    {
        // GetEyeData takes around 4ms to finish, so we run it on a thread.
        QMutexLocker locker(&_viveProEyeReadThread->eyeDataMutex);
        memcpy(&eyeDataBuffer, &_viveProEyeReadThread->eyeDataBuffer, sizeof(eyeDataBuffer));
    }

    if (eyeDataBuffer.getEyeDataResult != ViveSR::Error::WORK) {
        invalidateEyeInputs();
        return;
    }

    // only update from buffer values if the new data is "valid"
    if (!eyeDataBuffer.leftDirectionValid) {
        eyeDataBuffer.leftEyeGaze = _prevEyeData.leftEyeGaze;
        eyeDataBuffer.leftDirectionValid = _prevEyeData.leftDirectionValid;
    }
    if (!eyeDataBuffer.rightDirectionValid) {
        eyeDataBuffer.rightEyeGaze = _prevEyeData.rightEyeGaze;
        eyeDataBuffer.rightDirectionValid = _prevEyeData.rightDirectionValid;
    }
    if (!eyeDataBuffer.leftOpennessValid) {
        eyeDataBuffer.leftEyeOpenness = _prevEyeData.leftEyeOpenness;
        eyeDataBuffer.leftOpennessValid = _prevEyeData.leftOpennessValid;
    }
    if (!eyeDataBuffer.rightOpennessValid) {
        eyeDataBuffer.rightEyeOpenness = _prevEyeData.rightEyeOpenness;
        eyeDataBuffer.rightOpennessValid = _prevEyeData.rightOpennessValid;
    }
    _prevEyeData = eyeDataBuffer;

    // transform data into what the controller system expects.

    // in the data from sranipal, left=+x, up=+y, forward=+z
    mat4 localLeftEyeMat = glm::lookAt(vec3(0.0f, 0.0f, 0.0f),
                                       glm::vec3(eyeDataBuffer.leftEyeGaze[0],
                                                 eyeDataBuffer.leftEyeGaze[1],
                                                 -eyeDataBuffer.leftEyeGaze[2]),
                                       vec3(0.0f, 1.0f, 0.0f));
    quat localLeftEyeRot = glm::quat_cast(localLeftEyeMat);
    quat avatarLeftEyeRot = _inputDevice->_poseStateMap[controller::HEAD].rotation * localLeftEyeRot;

    mat4 localRightEyeMat = glm::lookAt(vec3(0.0f, 0.0f, 0.0f),
                                        glm::vec3(eyeDataBuffer.rightEyeGaze[0],
                                                  eyeDataBuffer.rightEyeGaze[1],
                                                  -eyeDataBuffer.rightEyeGaze[2]),
                                        vec3(0.0f, 1.0f, 0.0f));
    quat localRightEyeRot = glm::quat_cast(localRightEyeMat);
    quat avatarRightEyeRot = _inputDevice->_poseStateMap[controller::HEAD].rotation * localRightEyeRot;

    // TODO -- figure out translations for eyes
    if (eyeDataBuffer.leftDirectionValid) {
        _inputDevice->_poseStateMap[controller::LEFT_EYE] = controller::Pose(glm::vec3(), avatarLeftEyeRot);
        _inputDevice->_poseStateMap[controller::LEFT_EYE].valid = true;
    } else {
        _inputDevice->_poseStateMap[controller::LEFT_EYE].valid = false;
    }
    if (eyeDataBuffer.rightDirectionValid) {
        _inputDevice->_poseStateMap[controller::RIGHT_EYE] = controller::Pose(glm::vec3(), avatarRightEyeRot);
        _inputDevice->_poseStateMap[controller::RIGHT_EYE].valid = true;
    } else {
        _inputDevice->_poseStateMap[controller::RIGHT_EYE].valid = false;
    }

    quint64 now = usecTimestampNow();

    // in hifi, 0 is open 1 is closed.  in SRanipal 1 is open, 0 is closed.
    if (eyeDataBuffer.leftOpennessValid) {
        _inputDevice->_axisStateMap[controller::EYEBLINK_L] =
            controller::AxisValue(1.0f - eyeDataBuffer.leftEyeOpenness, now);
    } else {
        _inputDevice->_poseStateMap[controller::EYEBLINK_L].valid = false;
    }
    if (eyeDataBuffer.rightOpennessValid) {
        _inputDevice->_axisStateMap[controller::EYEBLINK_R] =
            controller::AxisValue(1.0f - eyeDataBuffer.rightEyeOpenness, now);
    } else {
        _inputDevice->_poseStateMap[controller::EYEBLINK_R].valid = false;
    }
}

glm::vec3 ViveControllerManager::getRollingAverageHandPoint(int handIndex, int pointIndex) const {
#if 0
    return _handPoints[0][handIndex][pointIndex];
#else
    glm::vec3 result;
    for (int s = 0; s < NUMBER_OF_HAND_TRACKER_SMOOTHING_FRAMES; s++) {
        result += _handPoints[s][handIndex][pointIndex];
    }
    return result / NUMBER_OF_HAND_TRACKER_SMOOTHING_FRAMES;
#endif
}


controller::Pose ViveControllerManager::trackedHandDataToPose(int hand, const glm::vec3& palmFacing,
                                                              int nearHandPositionIndex, int farHandPositionIndex) {
    glm::vec3 nearPoint = getRollingAverageHandPoint(hand, nearHandPositionIndex);

    glm::quat poseRot;
    if (nearHandPositionIndex != farHandPositionIndex) {
        glm::vec3 farPoint = getRollingAverageHandPoint(hand, farHandPositionIndex);

        glm::vec3 pointingDir = farPoint - nearPoint; // y axis
        glm::vec3 otherAxis = glm::cross(pointingDir, palmFacing);

        glm::mat4 rotMat;
        rotMat = glm::mat4(glm::vec4(otherAxis, 0.0f),
                           glm::vec4(pointingDir, 0.0f),
                           glm::vec4(palmFacing * (hand == 0 ? 1.0f : -1.0f), 0.0f),
                           glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        poseRot = glm::normalize(glmExtractRotation(rotMat));
    }

    if (!isNaN(poseRot)) {
        controller::Pose pose(nearPoint, poseRot);
        return pose;
    } else {
        controller::Pose pose;
        pose.valid = false;
        return pose;
    }
}


void ViveControllerManager::trackFinger(int hand, int jointIndex1, int jointIndex2, int jointIndex3, int jointIndex4,
                                        controller::StandardPoseChannel joint1, controller::StandardPoseChannel joint2,
                                        controller::StandardPoseChannel joint3, controller::StandardPoseChannel joint4) {

    glm::vec3 point1 = getRollingAverageHandPoint(hand, jointIndex1);
    glm::vec3 point2 = getRollingAverageHandPoint(hand, jointIndex2);
    glm::vec3 point3 = getRollingAverageHandPoint(hand, jointIndex3);
    glm::vec3 point4 = getRollingAverageHandPoint(hand, jointIndex4);

    glm::vec3 wristPos = getRollingAverageHandPoint(hand, ViveHandJointIndex::HAND);
    glm::vec3 thumb2 = getRollingAverageHandPoint(hand, ViveHandJointIndex::THUMB_2);
    glm::vec3 pinkie1 = getRollingAverageHandPoint(hand, ViveHandJointIndex::PINKY_1);

    // 1st
    glm::vec3 palmFacing = glm::normalize(glm::cross(pinkie1 - wristPos, thumb2 - wristPos));
    glm::vec3 handForward = glm::normalize(point1 - wristPos);
    glm::vec3 x = glm::normalize(glm::cross(palmFacing, handForward));
    glm::vec3 y = glm::normalize(point2 - point1);
    glm::vec3 z = (hand == 0) ? glm::cross(y, x) : glm::cross(x, y);
    glm::mat4 rotMat1 = glm::mat4(glm::vec4(x, 0.0f),
                                  glm::vec4(y, 0.0f),
                                  glm::vec4(z, 0.0f),
                                  glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glm::quat rot1 = glm::normalize(glmExtractRotation(rotMat1));
    if (!isNaN(rot1)) {
        _inputDevice->_poseStateMap[joint1] = controller::Pose(point1, rot1);
    }


    // 2nd
    glm::vec3 x2 = x; // glm::normalize(glm::cross(point3 - point2, point2 - point1));
    glm::vec3 y2 = glm::normalize(point3 - point2);
    glm::vec3 z2 = (hand == 0) ? glm::cross(y2, x2) : glm::cross(x2, y2);

    glm::mat4 rotMat2 = glm::mat4(glm::vec4(x2, 0.0f),
                                  glm::vec4(y2, 0.0f),
                                  glm::vec4(z2, 0.0f),
                                  glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glm::quat rot2 = glm::normalize(glmExtractRotation(rotMat2));
    if (!isNaN(rot2)) {
        _inputDevice->_poseStateMap[joint2] = controller::Pose(point2, rot2);
    }


    // 3rd
    glm::vec3 x3 = x; // glm::normalize(glm::cross(point4 - point3, point3 - point1));
    glm::vec3 y3 = glm::normalize(point4 - point3);
    glm::vec3 z3 = (hand == 0) ? glm::cross(y3, x3) : glm::cross(x3, y3);

    glm::mat4 rotMat3 = glm::mat4(glm::vec4(x3, 0.0f),
                                  glm::vec4(y3, 0.0f),
                                  glm::vec4(z3, 0.0f),
                                  glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glm::quat rot3 = glm::normalize(glmExtractRotation(rotMat3));
    if (!isNaN(rot3)) {
        _inputDevice->_poseStateMap[joint3] = controller::Pose(point3, rot3);
    }


    // 4th
    glm::quat rot4 = rot3;
    if (!isNaN(rot4)) {
        _inputDevice->_poseStateMap[joint4] = controller::Pose(point4, rot4);
    }
}


void ViveControllerManager::updateCameraHandTracker(float deltaTime,
                                                    const controller::InputCalibrationData& inputCalibrationData) {

    if (areBothHandControllersActive(_system)) {
        // if both hand-controllers are in use, don't do camera hand tracking
        disableGestureDetection();
    } else {
        enableGestureDetection();
    }

    if (!_viveCameraHandTracker) {
        return;
    }

    const GestureResult* results = NULL;
    int handTrackerFrameIndex { -1 };
    int resultsHandCount = GetGestureResult(&results, &handTrackerFrameIndex);

    // FIXME: Why the commented-out condition?
    if (handTrackerFrameIndex >= 0 /* && handTrackerFrameIndex != _lastHandTrackerFrameIndex */) {
#ifdef HAND_TRACKER_USE_EXTERNAL_TRANSFORM
        glm::mat4 trackedHandToAvatar =
            glm::inverse(inputCalibrationData.avatarMat) *
            inputCalibrationData.sensorToWorldMat *
            inputCalibrationData.hmdSensorMat;
        // glm::mat4 trackedHandToAvatar = _inputDevice->_poseStateMap[controller::HEAD].getMatrix() * Matrices::Y_180;
#else
        DisplayPluginPointer displayPlugin = _container->getActiveDisplayPlugin();
        std::shared_ptr<OpenVrDisplayPlugin> openVRDisplayPlugin =
            std::dynamic_pointer_cast<OpenVrDisplayPlugin>(displayPlugin);
        glm::mat4 sensorResetMatrix;
        if (openVRDisplayPlugin) {
            sensorResetMatrix = openVRDisplayPlugin->getSensorResetMatrix();
        }

        glm::mat4 trackedHandToAvatar =
            glm::inverse(inputCalibrationData.avatarMat) *
            inputCalibrationData.sensorToWorldMat *
            sensorResetMatrix;
#endif

        // roll all the old points in the rolling average
        memmove(&(_handPoints[1]),
                &(_handPoints[0]),
                sizeof(_handPoints[0]) * (NUMBER_OF_HAND_TRACKER_SMOOTHING_FRAMES - 1));

        for (int handIndex = 0; handIndex < resultsHandCount; handIndex++) {
            bool isLeftHand = results[handIndex].isLeft;

            vr::ETrackedControllerRole controllerRole =
                isLeftHand ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand;
            if (isHandControllerActive(_system, controllerRole)) {
                continue; // if the controller for this hand is tracked, ignore camera hand tracking
            }

            int hand =  isLeftHand ? 0 : 1;
            for (int pointIndex = 0; pointIndex < NUMBER_OF_HAND_POINTS; pointIndex++) {
                glm::vec3 pos(results[handIndex].points[3 * pointIndex],
                              results[handIndex].points[3 * pointIndex + 1],
                              -results[handIndex].points[3 * pointIndex + 2]);
                _handPoints[0][hand][pointIndex] = transformPoint(trackedHandToAvatar, pos);
            }

            glm::vec3 wristPos = getRollingAverageHandPoint(hand, ViveHandJointIndex::HAND);
            glm::vec3 thumb2 = getRollingAverageHandPoint(hand, ViveHandJointIndex::THUMB_2);
            glm::vec3 pinkie1 = getRollingAverageHandPoint(hand, ViveHandJointIndex::PINKY_1);
            glm::vec3 palmFacing = glm::cross(pinkie1 - wristPos, thumb2 - wristPos); // z axis

            _inputDevice->_poseStateMap[isLeftHand ? controller::LEFT_HAND : controller::RIGHT_HAND] =
                trackedHandDataToPose(hand, palmFacing, ViveHandJointIndex::HAND, ViveHandJointIndex::MIDDLE_1);
            trackFinger(hand, ViveHandJointIndex::THUMB_1, ViveHandJointIndex::THUMB_2, ViveHandJointIndex::THUMB_3,
                        ViveHandJointIndex::THUMB_4,
                        isLeftHand ? controller::LEFT_HAND_THUMB1 : controller::RIGHT_HAND_THUMB1,
                        isLeftHand ? controller::LEFT_HAND_THUMB2 : controller::RIGHT_HAND_THUMB2,
                        isLeftHand ? controller::LEFT_HAND_THUMB3 : controller::RIGHT_HAND_THUMB3,
                        isLeftHand ? controller::LEFT_HAND_THUMB4 : controller::RIGHT_HAND_THUMB4);
            trackFinger(hand, ViveHandJointIndex::INDEX_1, ViveHandJointIndex::INDEX_2, ViveHandJointIndex::INDEX_3,
                        ViveHandJointIndex::INDEX_4,
                        isLeftHand ? controller::LEFT_HAND_INDEX1 : controller::RIGHT_HAND_INDEX1,
                        isLeftHand ? controller::LEFT_HAND_INDEX2 : controller::RIGHT_HAND_INDEX2,
                        isLeftHand ? controller::LEFT_HAND_INDEX3 : controller::RIGHT_HAND_INDEX3,
                        isLeftHand ? controller::LEFT_HAND_INDEX4 : controller::RIGHT_HAND_INDEX4);
            trackFinger(hand, ViveHandJointIndex::MIDDLE_1, ViveHandJointIndex::MIDDLE_2, ViveHandJointIndex::MIDDLE_3,
                        ViveHandJointIndex::MIDDLE_4,
                        isLeftHand ? controller::LEFT_HAND_MIDDLE1 : controller::RIGHT_HAND_MIDDLE1,
                        isLeftHand ? controller::LEFT_HAND_MIDDLE2 : controller::RIGHT_HAND_MIDDLE2,
                        isLeftHand ? controller::LEFT_HAND_MIDDLE3 : controller::RIGHT_HAND_MIDDLE3,
                        isLeftHand ? controller::LEFT_HAND_MIDDLE4 : controller::RIGHT_HAND_MIDDLE4);
            trackFinger(hand, ViveHandJointIndex::RING_1, ViveHandJointIndex::RING_2, ViveHandJointIndex::RING_3,
                        ViveHandJointIndex::RING_4,
                        isLeftHand ? controller::LEFT_HAND_RING1 : controller::RIGHT_HAND_RING1,
                        isLeftHand ? controller::LEFT_HAND_RING2 : controller::RIGHT_HAND_RING2,
                        isLeftHand ? controller::LEFT_HAND_RING3 : controller::RIGHT_HAND_RING3,
                        isLeftHand ? controller::LEFT_HAND_RING4 : controller::RIGHT_HAND_RING4);
            trackFinger(hand, ViveHandJointIndex::PINKY_1, ViveHandJointIndex::PINKY_2, ViveHandJointIndex::PINKY_3,
                        ViveHandJointIndex::PINKY_4,
                        isLeftHand ? controller::LEFT_HAND_PINKY1 : controller::RIGHT_HAND_PINKY1,
                        isLeftHand ? controller::LEFT_HAND_PINKY2 : controller::RIGHT_HAND_PINKY2,
                        isLeftHand ? controller::LEFT_HAND_PINKY3 : controller::RIGHT_HAND_PINKY3,
                        isLeftHand ? controller::LEFT_HAND_PINKY4 : controller::RIGHT_HAND_PINKY4);
        }
    }
    _lastHandTrackerFrameIndex = handTrackerFrameIndex;
}
#endif


void ViveControllerManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {

    if (!_system) {
        return;
    }

    if (isDesktopMode() && _desktopMode) {
        _system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, _nextSimPoseData.vrPoses, vr::k_unMaxTrackedDeviceCount);
        _nextSimPoseData.update(Matrices::IDENTITY);
    } else if (isDesktopMode()) {
        _nextSimPoseData.resetToInvalid();
    }

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    handleOpenVrEvents();
    if (openVrQuitRequested()) {
        deactivate();
        return;
    }

    // because update mutates the internal state we need to lock
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData);
    });

    if (_inputDevice->_trackedControllers == 0 && _registeredWithInputMapper) {
        userInputMapper->removeDevice(_inputDevice->_deviceID);
        _registeredWithInputMapper = false;
        _inputDevice->_poseStateMap.clear();
    }

    if (!_registeredWithInputMapper && _inputDevice->_trackedControllers > 0) {
        userInputMapper->registerDevice(_inputDevice);
        _registeredWithInputMapper = true;
    }

#ifdef VIVE_PRO_EYE
    if (_viveProEye && _eyeTrackingEnabled) {
        updateEyeTracker(deltaTime, inputCalibrationData);
    }

    updateCameraHandTracker(deltaTime, inputCalibrationData);
#endif

}

void ViveControllerManager::loadSettings() {
    Settings settings;
    QString nameString = getName();
    settings.beginGroup(nameString);
    {
        if (_inputDevice) {
            const double DEFAULT_ARM_CIRCUMFERENCE = 0.33;
            const double DEFAULT_SHOULDER_WIDTH = 0.48;
            const QString DEFAULT_OUT_OF_RANGE_STRATEGY = "DropAfterDelay";
            _inputDevice->_armCircumference = settings.value("armCircumference", QVariant(DEFAULT_ARM_CIRCUMFERENCE)).toDouble();
            _inputDevice->_shoulderWidth = settings.value("shoulderWidth", QVariant(DEFAULT_SHOULDER_WIDTH)).toDouble();
            _inputDevice->_outOfRangeDataStrategy = stringToOutOfRangeDataStrategy(settings.value("outOfRangeDataStrategy", QVariant(DEFAULT_OUT_OF_RANGE_STRATEGY)).toString());
        }

        const bool DEFAULT_EYE_TRACKING_ENABLED = false;
        _eyeTrackingEnabled = settings.value("eyeTrackingEnabled", QVariant(DEFAULT_EYE_TRACKING_ENABLED)).toBool();
    }
    settings.endGroup();
}

void ViveControllerManager::saveSettings() const {
    Settings settings;
    QString nameString = getName();
    settings.beginGroup(nameString);
    {
        if (_inputDevice) {
            settings.setValue(QString("armCircumference"), _inputDevice->_armCircumference);
            settings.setValue(QString("shoulderWidth"), _inputDevice->_shoulderWidth);
            settings.setValue(QString("outOfRangeDataStrategy"), outOfRangeDataStrategyToString(_inputDevice->_outOfRangeDataStrategy));
        }

        settings.setValue(QString("eyeTrackingEnabled"), _eyeTrackingEnabled);
    }
    settings.endGroup();
}


ViveControllerManager::InputDevice::InputDevice(vr::IVRSystem*& system) :
    controller::InputDevice("Vive"),
    _system(system) {

    _configStringMap[Config::None] = QString("None");
    _configStringMap[Config::Feet] = QString("Feet");
    _configStringMap[Config::FeetAndHips] = QString("FeetAndHips");
    _configStringMap[Config::FeetHipsAndChest] = QString("FeetHipsAndChest");
    _configStringMap[Config::FeetHipsAndShoulders] = QString("FeetHipsAndShoulders");
    _configStringMap[Config::FeetHipsChestAndShoulders] = QString("FeetHipsChestAndShoulders");
}

void ViveControllerManager::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _poseStateMap.clear();
    _buttonPressedMap.clear();
    _validTrackedObjects.clear();
    _trackedControllers = 0;

    if (_headsetName == "") {
        _headsetName = getOpenVrDeviceName();
        if (_headsetName == "HTC") {
            _headsetName += " Vive";
        }
        if (oculusViaOpenVR()) {
            _headsetName = "OpenVR";  // Enables calibration dialog to function when debugging using Oculus.
        }
    }
    // While the keyboard is open, we defer strictly to the keyboard values
    if (isOpenVrKeyboardShown()) {
        _axisStateMap.clear();
        return;
    }

    PerformanceTimer perfTimer("ViveControllerManager::update");

    auto leftHandDeviceIndex = _system->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
    auto rightHandDeviceIndex = _system->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);

    handleHandController(deltaTime, leftHandDeviceIndex, inputCalibrationData, true);
    handleHandController(deltaTime, rightHandDeviceIndex, inputCalibrationData, false);

    // collect poses for all generic trackers
    for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
        handleTrackedObject(i, inputCalibrationData);
        handleHmd(i, inputCalibrationData);
    }

    // handle haptics
    {
        Locker locker(_lock);
        if (_leftHapticDuration > 0.0f) {
            hapticsHelper(deltaTime, true);
        }
        if (_rightHapticDuration > 0.0f) {
            hapticsHelper(deltaTime, false);
        }
    }

    if (leftHandDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
        _trackedControllers++;
    }
    if (rightHandDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
        _trackedControllers++;
    }

    calibrateFromHandController(inputCalibrationData);
    calibrateFromUI(inputCalibrationData);

    updateCalibratedLimbs(inputCalibrationData);
    _lastSimPoseData = _nextSimPoseData;
}

void ViveControllerManager::InputDevice::calibrateFromHandController(const controller::InputCalibrationData& inputCalibrationData) {
    if (checkForCalibrationEvent()) {
        quint64 currentTime = usecTimestampNow();
        if (!_timeTilCalibrationSet) {
            _timeTilCalibrationSet = true;
            _timeTilCalibration = currentTime + CALIBRATION_TIMELAPSE;
        }

        if (currentTime > _timeTilCalibration && !_triggersPressedHandled) {
            _triggersPressedHandled = true;
            calibrateOrUncalibrate(inputCalibrationData);
        }
    } else {
        _triggersPressedHandled = false;
        _timeTilCalibrationSet = false;
    }
}

void ViveControllerManager::InputDevice::calibrateFromUI(const controller::InputCalibrationData& inputCalibrationData) {
    if (_calibrate) {
        uncalibrate();
        calibrate(inputCalibrationData);
        emitCalibrationStatus();
        _calibrate = false;
    }
}

static const float CM_TO_M = 0.01f;
static const float M_TO_CM = 100.0f;

void ViveControllerManager::InputDevice::configureCalibrationSettings(const QJsonObject configurationSettings) {
    Locker locker(_lock);
    if (!configurationSettings.empty()) {
        auto iter = configurationSettings.begin();
        auto end = configurationSettings.end();
        bool hmdDesktopTracking = true;
        bool hmdDesktopMode = false;
        while (iter != end) {
            if (iter.key() == "bodyConfiguration") {
                setConfigFromString(iter.value().toString());
            } else if (iter.key() == "headConfiguration") {
                QJsonObject headObject = iter.value().toObject();
                bool overrideHead = headObject["override"].toBool();
                if (overrideHead) {
                    _headConfig = HeadConfig::Puck;
                    _headPuckYOffset = (float)headObject["Y"].toDouble() * CM_TO_M;
                    _headPuckZOffset = (float)headObject["Z"].toDouble() * CM_TO_M;
                } else {
                    _headConfig = HeadConfig::HMD;
                }
            } else if (iter.key() == "handConfiguration") {
                QJsonObject handsObject = iter.value().toObject();
                bool overrideHands = handsObject["override"].toBool();
                if (overrideHands) {
                    _handConfig = HandConfig::Pucks;
                    _handPuckYOffset = (float)handsObject["Y"].toDouble() * CM_TO_M;
                    _handPuckZOffset = (float)handsObject["Z"].toDouble() * CM_TO_M;
                } else {
                    _handConfig = HandConfig::HandController;
                }
            } else if (iter.key() == "armCircumference") {
                _armCircumference = (float)iter.value().toDouble() * CM_TO_M;
            } else if (iter.key() == "shoulderWidth") {
                _shoulderWidth = (float)iter.value().toDouble() * CM_TO_M;
            } else if (iter.key() == "hmdDesktopTracking") {
                hmdDesktopTracking = iter.value().toBool();
            } else if (iter.key() == "desktopMode") {
                hmdDesktopMode = iter.value().toBool();
            } else if (iter.key() == "outOfRangeDataStrategy") {
                _outOfRangeDataStrategy = stringToOutOfRangeDataStrategy(iter.value().toString());
            }
            iter++;
        }

        _hmdTrackingEnabled = !(hmdDesktopMode && hmdDesktopTracking);
    }
}

void ViveControllerManager::InputDevice::calibrateNextFrame() {
    Locker locker(_lock);
    _calibrate = true;
}

QJsonObject ViveControllerManager::InputDevice::configurationSettings() {
    Locker locker(_lock);
    QJsonObject configurationSettings;
    configurationSettings["trackerConfiguration"] = configToString(_preferedConfig);
    configurationSettings["HMDHead"] = (_headConfig == HeadConfig::HMD);
    configurationSettings["handController"] = (_handConfig == HandConfig::HandController);
    configurationSettings["puckCount"] = (int)_validTrackedObjects.size();
    configurationSettings["armCircumference"] = (double)(_armCircumference * M_TO_CM);
    configurationSettings["shoulderWidth"] = (double)(_shoulderWidth * M_TO_CM);
    configurationSettings["outOfRangeDataStrategy"] = outOfRangeDataStrategyToString(_outOfRangeDataStrategy);
    return configurationSettings;
}

void ViveControllerManager::InputDevice::emitCalibrationStatus() {
    auto inputConfiguration = DependencyManager::get<InputConfiguration>();
    QJsonObject status = QJsonObject();
    status["calibrated"] = _calibrated;
    status["configuration"] = configToString(_preferedConfig);
    status["head_puck"] = (_headConfig == HeadConfig::Puck);
    status["hand_pucks"] = (_handConfig == HandConfig::Pucks);
    status["puckCount"] = (int)_validTrackedObjects.size();
    status["UI"] = _calibrate;

    emit inputConfiguration->calibrationStatus(status);
}

static controller::Pose buildPose(const glm::mat4& mat, const glm::vec3& linearVelocity, const glm::vec3& angularVelocity) {
    return controller::Pose(extractTranslation(mat), glmExtractRotation(mat), linearVelocity, angularVelocity);
}

void ViveControllerManager::InputDevice::handleTrackedObject(uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData) {
    uint32_t poseIndex = controller::TRACKED_OBJECT_00 + deviceIndex;
    printDeviceTrackingResultChange(deviceIndex);
    if (_system->IsTrackedDeviceConnected(deviceIndex) &&
        _system->GetTrackedDeviceClass(deviceIndex) == vr::TrackedDeviceClass_GenericTracker &&
        _nextSimPoseData.vrPoses[deviceIndex].bPoseIsValid &&
        poseIndex <= controller::TRACKED_OBJECT_15) {

        uint64_t now = usecTimestampNow();
        controller::Pose pose;
        switch (_outOfRangeDataStrategy) {
        case OutOfRangeDataStrategy::Drop:
        default:
            // Drop - Mark all non Running_OK results as invald
            if (_nextSimPoseData.vrPoses[deviceIndex].eTrackingResult == vr::TrackingResult_Running_OK) {
                pose = buildPose(_nextSimPoseData.poses[deviceIndex], _nextSimPoseData.linearVelocities[deviceIndex], _nextSimPoseData.angularVelocities[deviceIndex]);
            } else {
                pose.valid = false;
            }
            break;
        case OutOfRangeDataStrategy::None:
            // None - Ignore eTrackingResult all together
            pose = buildPose(_nextSimPoseData.poses[deviceIndex], _nextSimPoseData.linearVelocities[deviceIndex], _nextSimPoseData.angularVelocities[deviceIndex]);
            break;
        case OutOfRangeDataStrategy::Freeze:
            // Freeze - Dont invalide non Running_OK poses, instead just return the last good pose.
            if (_nextSimPoseData.vrPoses[deviceIndex].eTrackingResult == vr::TrackingResult_Running_OK) {
                pose = buildPose(_nextSimPoseData.poses[deviceIndex], _nextSimPoseData.linearVelocities[deviceIndex], _nextSimPoseData.angularVelocities[deviceIndex]);
            } else {
                pose = buildPose(_lastSimPoseData.poses[deviceIndex], _lastSimPoseData.linearVelocities[deviceIndex], _lastSimPoseData.angularVelocities[deviceIndex]);

                // make sure that we do not overwrite the pose in the _lastSimPose with incorrect data.
                _nextSimPoseData.poses[deviceIndex] = _lastSimPoseData.poses[deviceIndex];
                _nextSimPoseData.linearVelocities[deviceIndex] = _lastSimPoseData.linearVelocities[deviceIndex];
                _nextSimPoseData.angularVelocities[deviceIndex] = _lastSimPoseData.angularVelocities[deviceIndex];
            }
            break;
        case OutOfRangeDataStrategy::DropAfterDelay:
            const uint64_t DROP_DELAY_TIME = 500 * USECS_PER_MSEC;

            // All Running_OK results are valid.
            if (_nextSimPoseData.vrPoses[deviceIndex].eTrackingResult == vr::TrackingResult_Running_OK) {
                pose = buildPose(_nextSimPoseData.poses[deviceIndex], _nextSimPoseData.linearVelocities[deviceIndex], _nextSimPoseData.angularVelocities[deviceIndex]);
                // update the timer
                _simDataRunningOkTimestampMap[deviceIndex] = now;
            } else if (now - _simDataRunningOkTimestampMap[deviceIndex] < DROP_DELAY_TIME) {
                // report the pose, even though pose is out-of-range
                pose = buildPose(_nextSimPoseData.poses[deviceIndex], _nextSimPoseData.linearVelocities[deviceIndex], _nextSimPoseData.angularVelocities[deviceIndex]);
            } else {
                // this pose has been out-of-range for too long.
                pose.valid = false;
            }
            break;
        }

        if (pose.valid) {
            // transform into avatar frame
            glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
            _poseStateMap[poseIndex] = pose.transform(controllerToAvatar);

            // but _validTrackedObjects remain in sensor frame
            _validTrackedObjects.push_back(std::make_pair(poseIndex, pose));
            _trackedControllers++;
        } else {
            // insert invalid pose into state map
            _poseStateMap[poseIndex] = pose;
        }
    } else {
        controller::Pose invalidPose;
        _poseStateMap[poseIndex] = invalidPose;
    }
}

void ViveControllerManager::InputDevice::sendUserActivityData(QString activity) {
    QJsonObject jsonData = {
        {"num_pucks", (int)_validTrackedObjects.size()},
        {"configuration", configToString(_preferedConfig)},
        {"head_puck", (_headConfig == HeadConfig::Puck) ? true : false},
        {"hand_pucks", (_handConfig == HandConfig::Pucks) ? true : false}
    };

    UserActivityLogger::getInstance().logAction(activity, jsonData);
}

void ViveControllerManager::InputDevice::calibrateOrUncalibrate(const controller::InputCalibrationData& inputCalibration) {
    if (!_calibrated) {
        calibrate(inputCalibration);
        if (_calibrated) {
            sendUserActivityData("mocap_button_success");
        } else {
            sendUserActivityData("mocap_button_fail");
        }
        emitCalibrationStatus();
    } else {
        uncalibrate();
        sendUserActivityData("mocap_button_uncalibrate");
    }
}

void ViveControllerManager::InputDevice::calibrate(const controller::InputCalibrationData& inputCalibration) {
    qDebug() << "Puck Calibration: Starting...";

    int puckCount = (int)_validTrackedObjects.size();
    qDebug() << "Puck Calibration: " << puckCount << " pucks found for calibration";

    if (puckCount == 0) {
        uncalibrate();
        return;
    }

    // Compute the defaultToRefrenceMat, this will take inputCalibration default poses into the reference frame. (sensor space)
    glm::mat4 defaultToReferenceMat = glm::mat4();
    if (_headConfig == HeadConfig::HMD) {
        defaultToReferenceMat = calculateDefaultToReferenceForHmd(inputCalibration);
    } else if (_headConfig == HeadConfig::Puck) {
        std::sort(_validTrackedObjects.begin(), _validTrackedObjects.end(), sortPucksYPosition);
        defaultToReferenceMat = calculateDefaultToReferenceForHeadPuck(inputCalibration);
    }

    _config = _preferedConfig;

    bool headConfigured = configureHead(defaultToReferenceMat, inputCalibration);
    bool handsConfigured = configureHands(defaultToReferenceMat, inputCalibration);
    bool bodyConfigured = configureBody(defaultToReferenceMat, inputCalibration);

    if (!headConfigured || !handsConfigured || !bodyConfigured) {
        uncalibrate();
    } else {
        _calibrated = true;
        qDebug() << "PuckCalibration: " << configToString(_config) << " Configuration Successful";
    }
}

bool ViveControllerManager::InputDevice::configureHands(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {

    // Sort valid tracked objects in the default frame by the x dimension (left to right).
    // Because the sort is in the default frame we guarentee that poses are relative to the head facing.
    // i.e. -x will always be to the left of the head, and +x will be to the right.
    // This allows the user to be facing in any direction in sensor space while calibrating.
    glm::mat4 referenceToDefaultMat = glm::inverse(defaultToReferenceMat);
    std::sort(_validTrackedObjects.begin(), _validTrackedObjects.end(), [&referenceToDefaultMat](const PuckPosePair& a, const PuckPosePair& b) {
        glm::vec3 aPos = transformPoint(referenceToDefaultMat, a.second.translation);
        glm::vec3 bPos = transformPoint(referenceToDefaultMat, b.second.translation);
        return (aPos.x < bPos.x);
    });
    int puckCount = (int)_validTrackedObjects.size();
    if (_handConfig == HandConfig::Pucks && puckCount >= MIN_PUCK_COUNT) {
        glm::vec3 headXAxis = getReferenceHeadXAxis(defaultToReferenceMat, inputCalibration.defaultHeadMat);
        glm::vec3 headPosition = getReferenceHeadPosition(defaultToReferenceMat, inputCalibration.defaultHeadMat);
        size_t FIRST_INDEX = 0;
        size_t LAST_INDEX = _validTrackedObjects.size() -1;
        auto& firstHand = _validTrackedObjects[FIRST_INDEX];
        auto& secondHand = _validTrackedObjects[LAST_INDEX];
        controller::Pose& firstHandPose = firstHand.second;
        controller::Pose& secondHandPose = secondHand.second;

        if (determineLimbOrdering(firstHandPose, secondHandPose, headXAxis, headPosition)) {
            calibrateLeftHand(defaultToReferenceMat, inputCalibration, firstHand);
            calibrateRightHand(defaultToReferenceMat, inputCalibration, secondHand);
            _validTrackedObjects.erase(_validTrackedObjects.begin());
            _validTrackedObjects.erase(_validTrackedObjects.end() - 1);
            _overrideHands = true;
            return true;
        } else {
            calibrateLeftHand(defaultToReferenceMat, inputCalibration, secondHand);
            calibrateRightHand(defaultToReferenceMat, inputCalibration, firstHand);
            _validTrackedObjects.erase(_validTrackedObjects.begin());
            _validTrackedObjects.erase(_validTrackedObjects.end() - 1);
            _overrideHands = true;
            return true;
        }
    } else if (_handConfig == HandConfig::HandController) {
        _overrideHands = false;
        return true;
    }
    return false;
}

bool ViveControllerManager::InputDevice::configureHead(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    std::sort(_validTrackedObjects.begin(), _validTrackedObjects.end(), sortPucksYPosition);
    int puckCount = (int)_validTrackedObjects.size();
    if (_headConfig == HeadConfig::Puck && puckCount >= MIN_HEAD) {
         calibrateHead(defaultToReferenceMat, inputCalibration);
        _validTrackedObjects.erase(_validTrackedObjects.end() - 1);
        _overrideHead = true;
        return true;
    } else if (_headConfig == HeadConfig::HMD) {
        return true;
    }
    return false;
}

bool ViveControllerManager::InputDevice::configureBody(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    std::sort(_validTrackedObjects.begin(), _validTrackedObjects.end(), sortPucksYPosition);
    int puckCount = (int)_validTrackedObjects.size();
    if (_config == Config::None) {
        return true;
    } else if (_config == Config::Feet && puckCount >= MIN_PUCK_COUNT) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
        return true;
    } else if (_config == Config::FeetAndHips && puckCount >= MIN_FEET_AND_HIPS) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
        calibrateHips(defaultToReferenceMat, inputCalibration);
        return true;
    } else if (_config == Config::FeetHipsAndChest && puckCount >= MIN_FEET_HIPS_CHEST) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
        calibrateHips(defaultToReferenceMat, inputCalibration);
        calibrateChest(defaultToReferenceMat, inputCalibration);
        return true;
    } else if (_config == Config::FeetHipsAndShoulders && puckCount >= MIN_FEET_HIPS_SHOULDERS) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
        calibrateHips(defaultToReferenceMat, inputCalibration);
        int firstShoulderIndex = 3;
        int secondShoulderIndex = 4;
        calibrateShoulders(defaultToReferenceMat, inputCalibration, firstShoulderIndex, secondShoulderIndex);
        return true;
    } else if (_config == Config::FeetHipsChestAndShoulders && puckCount >= MIN_FEET_HIPS_CHEST_SHOULDERS) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
        calibrateHips(defaultToReferenceMat, inputCalibration);
        calibrateChest(defaultToReferenceMat, inputCalibration);
        int firstShoulderIndex = 4;
        int secondShoulderIndex = 5;
        calibrateShoulders(defaultToReferenceMat, inputCalibration, firstShoulderIndex, secondShoulderIndex);
        return true;
    }
    qDebug() << "Puck Calibration: " << configToString(_config) << " Config Failed: Could not meet the minimal # of pucks";
    return false;
}

void ViveControllerManager::InputDevice::uncalibrate() {
    _config = Config::None;
    _pucksPostOffset.clear();
    _pucksPreOffset.clear();
    _jointToPuckMap.clear();
    _calibrated = false;
    _overrideHead = false;
    _overrideHands = false;
}

void ViveControllerManager::InputDevice::updateCalibratedLimbs(const controller::InputCalibrationData& inputCalibration) {
    _poseStateMap[controller::LEFT_FOOT] = addOffsetToPuckPose(inputCalibration, controller::LEFT_FOOT);
    _poseStateMap[controller::RIGHT_FOOT] = addOffsetToPuckPose(inputCalibration, controller::RIGHT_FOOT);
    _poseStateMap[controller::HIPS] = addOffsetToPuckPose(inputCalibration, controller::HIPS);
    _poseStateMap[controller::SPINE2] = addOffsetToPuckPose(inputCalibration, controller::SPINE2);
    _poseStateMap[controller::RIGHT_ARM] = addOffsetToPuckPose(inputCalibration, controller::RIGHT_ARM);
    _poseStateMap[controller::LEFT_ARM] = addOffsetToPuckPose(inputCalibration, controller::LEFT_ARM);

    if (_overrideHead) {
        _poseStateMap[controller::HEAD] = addOffsetToPuckPose(inputCalibration, controller::HEAD);
    }

    if (_overrideHands) {
        _poseStateMap[controller::LEFT_HAND] = addOffsetToPuckPose(inputCalibration, controller::LEFT_HAND);
        _poseStateMap[controller::RIGHT_HAND] = addOffsetToPuckPose(inputCalibration, controller::RIGHT_HAND);
    }
}

controller::Pose ViveControllerManager::InputDevice::addOffsetToPuckPose(const controller::InputCalibrationData& inputCalibration, int joint) const {
    auto puck = _jointToPuckMap.find(joint);
    if (puck != _jointToPuckMap.end()) {
        uint32_t puckIndex = puck->second;

        // use sensor space pose.
        auto puckPoseIter = _validTrackedObjects.begin();
        while (puckPoseIter != _validTrackedObjects.end()) {
            if (puckPoseIter->first == puckIndex) {
                break;
            }
            puckPoseIter++;
        }

        if (puckPoseIter != _validTrackedObjects.end()) {

            glm::mat4 postMat; // identity
            auto postIter = _pucksPostOffset.find(puckIndex);
            if (postIter != _pucksPostOffset.end()) {
                postMat = postIter->second;
            }

            glm::mat4 preMat = glm::inverse(inputCalibration.avatarMat) * inputCalibration.sensorToWorldMat;
            auto preIter = _pucksPreOffset.find(puckIndex);
            if (preIter != _pucksPreOffset.end()) {
                preMat = preMat * preIter->second;
            }

            return puckPoseIter->second.postTransform(postMat).transform(preMat);
        }
    }
    return controller::Pose();
}

void ViveControllerManager::InputDevice::handleHmd(uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData) {
     if (_system->IsTrackedDeviceConnected(deviceIndex) &&
         _system->GetTrackedDeviceClass(deviceIndex) == vr::TrackedDeviceClass_HMD &&
         _nextSimPoseData.vrPoses[deviceIndex].bPoseIsValid) {

         if (_hmdTrackingEnabled){
             const mat4& mat = _nextSimPoseData.poses[deviceIndex];
             const vec3 linearVelocity = _nextSimPoseData.linearVelocities[deviceIndex];
             const vec3 angularVelocity = _nextSimPoseData.angularVelocities[deviceIndex];

             handleHeadPoseEvent(inputCalibrationData, mat, linearVelocity, angularVelocity);
         } else {
             const mat4& mat = mat4();
             const vec3 zero = vec3();
             handleHeadPoseEvent(inputCalibrationData, mat, zero, zero);
         }
         _trackedControllers++;
     }
}

void ViveControllerManager::InputDevice::handleHandController(float deltaTime, uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData, bool isLeftHand) {

    if (isDeviceIndexActive(_system, deviceIndex) && _nextSimPoseData.vrPoses[deviceIndex].bPoseIsValid) {

        // process pose
        const mat4& mat = _nextSimPoseData.poses[deviceIndex];
        const vec3 linearVelocity = _nextSimPoseData.linearVelocities[deviceIndex];
        const vec3 angularVelocity = _nextSimPoseData.angularVelocities[deviceIndex];
        handlePoseEvent(deltaTime, inputCalibrationData, mat, linearVelocity, angularVelocity, isLeftHand);

        vr::VRControllerState_t controllerState = vr::VRControllerState_t();
        if (_system->GetControllerState(deviceIndex, &controllerState, sizeof(vr::VRControllerState_t))) {
            // process each button
            for (uint32_t i = 0; i < vr::k_EButton_Max; ++i) {
                auto mask = vr::ButtonMaskFromId((vr::EVRButtonId)i);
                bool pressed = 0 != (controllerState.ulButtonPressed & mask);
                bool touched = 0 != (controllerState.ulButtonTouched & mask);
                handleButtonEvent(deltaTime, i, pressed, touched, isLeftHand);
            }

            // process each axis
            for (uint32_t i = 0; i < vr::k_unControllerStateAxisCount; i++) {
                handleAxisEvent(deltaTime, i, controllerState.rAxis[i].x, controllerState.rAxis[i].y, isLeftHand);
            }

            // pseudo buttons the depend on both of the above for-loops
            partitionTouchpad(controller::LS, controller::LX, controller::LY, controller::LS_CENTER, controller::LS_X, controller::LS_Y);
            partitionTouchpad(controller::RS, controller::RX, controller::RY, controller::RS_CENTER, controller::RS_X, controller::RS_Y);
        }
    }
}

// defaultToReferenceMat is an offset from avatar space to sensor space.
// it aligns the default center-eye in avatar space with the hmd in sensor space.
//
//  * E_a is the the default center-of-the-eyes transform in avatar space.
//  * E_s is the the hmd eye-center transform in sensor space, with roll and pitch removed.
//  * D is the defaultReferenceMat.
//
//  E_s = D * E_a  =>
//  D = E_s * inverse(E_a)
//
glm::mat4 ViveControllerManager::InputDevice::calculateDefaultToReferenceForHmd(const controller::InputCalibrationData& inputCalibration) {

    // the center-eye transform in avatar space.
    glm::mat4 E_a = inputCalibration.defaultCenterEyeMat;

    // the center-eye transform in sensor space.
    glm::mat4 E_s = inputCalibration.hmdSensorMat * Matrices::Y_180;  // the Y_180 is to convert hmd from -z forward to z forward.

    // cancel out roll and pitch on E_s
    glm::quat rot = cancelOutRollAndPitch(glmExtractRotation(E_s));
    glm::vec3 trans = extractTranslation(E_s);
    E_s = createMatFromQuatAndPos(rot, trans);

    return E_s * glm::inverse(E_a);
}

// defaultToReferenceMat is an offset from avatar space to sensor space.
// It aligns the default center-of-the-eyes transform in avatar space with the head-puck in sensor space.
// The offset from the center-of-the-eyes to the head-puck can be configured via _headPuckYOffset and _headPuckZOffset,
// These values are exposed in the configuration UI.
//
//  * E_a is the the default center-eye transform in avatar space.
//  * E_s is the the head-puck center-eye transform in sensor space, with roll and pitch removed.
//  * D is the defaultReferenceMat.
//
//  E_s = D * E_a  =>
//  D = E_s * inverse(E_a)
//
glm::mat4 ViveControllerManager::InputDevice::calculateDefaultToReferenceForHeadPuck(const controller::InputCalibrationData& inputCalibration) {

    // the center-eye transform in avatar space.
    glm::mat4 E_a = inputCalibration.defaultCenterEyeMat;

    // calculate the center-eye transform in sensor space, via the head-puck
    size_t headPuckIndex = _validTrackedObjects.size() - 1;
    controller::Pose headPuckPose = _validTrackedObjects[headPuckIndex].second;

    // AJT: TODO: handle case were forward is parallel with UNIT_Y.
    glm::vec3 forward = headPuckPose.rotation * -Vectors::UNIT_Z;
    glm::vec3 x = glm::normalize(glm::cross(Vectors::UNIT_Y, forward));
    glm::vec3 z = glm::normalize(glm::cross(x, Vectors::UNIT_Y));
    glm::mat3 centerEyeRotMat(x, Vectors::UNIT_Y, z);
    glm::vec3 centerEyeTrans = headPuckPose.translation + centerEyeRotMat * -glm::vec3(0.0f, _headPuckYOffset, _headPuckZOffset);

    glm::mat4 E_s(glm::vec4(centerEyeRotMat[0], 0.0f),
                  glm::vec4(centerEyeRotMat[1], 0.0f),
                  glm::vec4(centerEyeRotMat[2], 0.0f),
                  glm::vec4(centerEyeTrans, 1.0f));

    return E_s * glm::inverse(E_a);
}

void ViveControllerManager::InputDevice::partitionTouchpad(int sButton, int xAxis, int yAxis, int centerPseudoButton, int xPseudoButton, int yPseudoButton) {
    // Populate the L/RS_CENTER/OUTER pseudo buttons, corresponding to a partition of the L/RS space based on the X/Y values.
    const float CENTER_DEADBAND = 0.6f;
    const float DIAGONAL_DIVIDE_IN_RADIANS = PI / 4.0f;
    if (_buttonPressedMap.find(sButton) != _buttonPressedMap.end()) {
        float absX = abs(_axisStateMap[xAxis].value);
        float absY = abs(_axisStateMap[yAxis].value);
        glm::vec2 cartesianQuadrantI(absX, absY);
        float angle = glm::atan(cartesianQuadrantI.y / cartesianQuadrantI.x);
        float radius = glm::length(cartesianQuadrantI);
        bool isCenter = radius < CENTER_DEADBAND;
        _buttonPressedMap.insert(isCenter ? centerPseudoButton : ((angle < DIAGONAL_DIVIDE_IN_RADIANS) ? xPseudoButton :yPseudoButton));
    }
}

void ViveControllerManager::InputDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

// These functions do translation from the Steam IDs to the standard controller IDs
void ViveControllerManager::InputDevice::handleAxisEvent(float deltaTime, uint32_t axis, float x, float y, bool isLeftHand) {
    //FIX ME? It enters here every frame: probably we want to enter only if an event occurs
    axis += vr::k_EButton_Axis0;
    using namespace controller;

    if (axis == vr::k_EButton_SteamVR_Touchpad) {
        glm::vec2 stick(x, y);
        if (isLeftHand) {
            stick = _filteredLeftStick.process(deltaTime, stick);
        } else {
            stick = _filteredRightStick.process(deltaTime, stick);
        }
        _axisStateMap[isLeftHand ? LX : RX].value = stick.x;
        _axisStateMap[isLeftHand ? LY : RY].value = stick.y;
    } else if (axis == vr::k_EButton_SteamVR_Trigger) {
        _axisStateMap[isLeftHand ? LT : RT].value = x;
        // The click feeling on the Vive controller trigger represents a value of *precisely* 1.0,
        // so we can expose that as an additional button
        if (x >= 1.0f) {
            _buttonPressedMap.insert(isLeftHand ? LT_CLICK : RT_CLICK);
        }
    }
}

// An enum for buttons which do not exist in the StandardControls enum
enum ViveButtonChannel {
    LEFT_APP_MENU = controller::StandardButtonChannel::NUM_STANDARD_BUTTONS,
    RIGHT_APP_MENU
};

void ViveControllerManager::InputDevice::printDeviceTrackingResultChange(uint32_t deviceIndex) {
    if (_nextSimPoseData.vrPoses[deviceIndex].eTrackingResult != _lastSimPoseData.vrPoses[deviceIndex].eTrackingResult) {
        qDebug() << "OpenVR: Device" << deviceIndex << "Tracking Result changed from" <<
            deviceTrackingResultToString(_lastSimPoseData.vrPoses[deviceIndex].eTrackingResult)
                 << "to" << deviceTrackingResultToString(_nextSimPoseData.vrPoses[deviceIndex].eTrackingResult);
    }
}

bool ViveControllerManager::InputDevice::checkForCalibrationEvent() {
    auto endOfMap = _buttonPressedMap.end();
    auto leftTrigger = _buttonPressedMap.find(controller::LT);
    auto rightTrigger = _buttonPressedMap.find(controller::RT);
    auto leftAppButton = _buttonPressedMap.find(LEFT_APP_MENU);
    auto rightAppButton = _buttonPressedMap.find(RIGHT_APP_MENU);
    return ((leftTrigger != endOfMap && leftAppButton != endOfMap) && (rightTrigger != endOfMap && rightAppButton != endOfMap));
}

// These functions do translation from the Steam IDs to the standard controller IDs
void ViveControllerManager::InputDevice::handleButtonEvent(float deltaTime, uint32_t button, bool pressed, bool touched, bool isLeftHand) {

    using namespace controller;

    if (pressed) {
        if (button == vr::k_EButton_ApplicationMenu) {
            _buttonPressedMap.insert(isLeftHand ? LEFT_APP_MENU : RIGHT_APP_MENU);
        } else if (button == vr::k_EButton_Grip) {
            _axisStateMap[isLeftHand ? LEFT_GRIP : RIGHT_GRIP].value = 1.0f;
        } else if (button == vr::k_EButton_SteamVR_Trigger) {
            _buttonPressedMap.insert(isLeftHand ? LT : RT);
        } else if (button == vr::k_EButton_SteamVR_Touchpad) {
            _buttonPressedMap.insert(isLeftHand ? LS : RS);
        }
    } else {
        if (button == vr::k_EButton_Grip) {
            _axisStateMap[isLeftHand ? LEFT_GRIP : RIGHT_GRIP].value = 0.0f;
        }
    }

    if (touched) {
         if (button == vr::k_EButton_SteamVR_Touchpad) {
             _buttonPressedMap.insert(isLeftHand ? LS_TOUCH : RS_TOUCH);
        }
    }
}

void ViveControllerManager::InputDevice::handleHeadPoseEvent(const controller::InputCalibrationData& inputCalibrationData, const mat4& mat,
                                                             const vec3& linearVelocity, const vec3& angularVelocity) {
    //perform a 180 flip to make the HMD face the +z instead of -z, beacuse the head faces +z
    glm::mat4 matYFlip = mat * Matrices::Y_180;
    controller::Pose pose(extractTranslation(matYFlip), glmExtractRotation(matYFlip), linearVelocity, angularVelocity);

    glm::mat4 defaultHeadOffset;
    if (inputCalibrationData.hmdAvatarAlignmentType == controller::HmdAvatarAlignmentType::Eyes) {
        // align the eyes of the user with the eyes of the avatar
        defaultHeadOffset = glm::inverse(inputCalibrationData.defaultCenterEyeMat) * inputCalibrationData.defaultHeadMat;
    } else {
        // align the head of the user with the head of the avatar
        defaultHeadOffset = createMatFromQuatAndPos(Quaternions::IDENTITY, -DEFAULT_AVATAR_HEAD_TO_MIDDLE_EYE_OFFSET);
    }

    glm::mat4 sensorToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    _poseStateMap[controller::HEAD] = pose.postTransform(defaultHeadOffset).transform(sensorToAvatar);
}

void ViveControllerManager::InputDevice::handlePoseEvent(float deltaTime, const controller::InputCalibrationData& inputCalibrationData,
                                                         const mat4& mat, const vec3& linearVelocity,
                                                         const vec3& angularVelocity, bool isLeftHand) {
    auto pose = openVrControllerPoseToHandPose(isLeftHand, mat, linearVelocity, angularVelocity);

    // transform into avatar frame
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    _poseStateMap[isLeftHand ? controller::LEFT_HAND : controller::RIGHT_HAND] = pose.transform(controllerToAvatar);
}

bool ViveControllerManager::InputDevice::triggerHapticPulse(float strength, float duration, uint16_t index) {
    if (index > 2) {
        return false;
    }

    controller::Hand hand = (controller::Hand)index;

    Locker locker(_lock);
    if (hand == controller::BOTH || hand == controller::LEFT) {
        if (strength == 0.0f) {
            _leftHapticStrength = 0.0f;
            _leftHapticDuration = 0.0f;
        } else {
            _leftHapticStrength = (duration > _leftHapticDuration) ? strength : _leftHapticStrength;
            _leftHapticDuration = std::max(duration, _leftHapticDuration);
        }
    }
    if (hand == controller::BOTH || hand == controller::RIGHT) {
        if (strength == 0.0f) {
            _rightHapticStrength = 0.0f;
            _rightHapticDuration = 0.0f;
        } else {
            _rightHapticStrength = (duration > _rightHapticDuration) ? strength : _rightHapticStrength;
            _rightHapticDuration = std::max(duration, _rightHapticDuration);
        }
    }
    return true;
}

void ViveControllerManager::InputDevice::hapticsHelper(float deltaTime, bool leftHand) {
    auto handRole = leftHand ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand;
    auto deviceIndex = _system->GetTrackedDeviceIndexForControllerRole(handRole);

    if (_system->IsTrackedDeviceConnected(deviceIndex) &&
        _system->GetTrackedDeviceClass(deviceIndex) == vr::TrackedDeviceClass_Controller &&
        _nextSimPoseData.vrPoses[deviceIndex].bPoseIsValid) {
        float strength = leftHand ? _leftHapticStrength : _rightHapticStrength;
        float duration = leftHand ? _leftHapticDuration : _rightHapticDuration;

        // Vive Controllers only support duration up to 4 ms, which is short enough that any variation feels more like strength
        const float MAX_HAPTIC_TIME = 3999.0f; // in microseconds
        float hapticTime = strength * MAX_HAPTIC_TIME;
        if (hapticTime < duration * 1000.0f) {
            _system->TriggerHapticPulse(deviceIndex, 0, hapticTime);
        }

        float remainingHapticTime = duration - (hapticTime / 1000.0f + deltaTime * 1000.0f); // in milliseconds
        if (leftHand) {
            _leftHapticDuration = remainingHapticTime;
        } else {
            _rightHapticDuration = remainingHapticTime;
        }
    }
}

void ViveControllerManager::InputDevice::calibrateLeftHand(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, PuckPosePair& handPair) {
    controller::Pose& handPose = handPair.second;
    glm::vec3 handPoseZAxis = handPose.getRotation() * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 referenceHandYAxis = transformVectorFast(defaultToReferenceMat * inputCalibration.defaultLeftHand, glm::vec3(0.0f, 1.0f, 0.0f));
    const float EPSILON = 1.0e-4f;
    if (fabsf(fabsf(glm::dot(glm::normalize(referenceHandYAxis), glm::normalize(handPoseZAxis))) - 1.0f) < EPSILON) {
        handPoseZAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // This allows the user to not have to match the t-pose exactly.  We assume that the y facing of the hand lies in the plane of the puck.
    // Where the plane of the puck is defined by the the local z-axis of the puck, which is pointing out of the camera mount on the bottom of the puck.
    glm::vec3 zPrime = handPoseZAxis;
    glm::vec3 xPrime = glm::normalize(glm::cross(referenceHandYAxis, handPoseZAxis));
    glm::vec3 yPrime = glm::normalize(glm::cross(zPrime, xPrime));
    glm::mat4 newHandMat = glm::mat4(glm::vec4(xPrime, 0.0f), glm::vec4(yPrime, 0.0f),
                                     glm::vec4(zPrime, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    glm::quat initialRot = handPose.getRotation();
    glm::quat postOffsetRot = glm::inverse(initialRot) * glmExtractRotation(newHandMat);
    glm::vec3 postOffsetTrans = postOffsetRot * -glm::vec3(0.0f, _handPuckYOffset, _handPuckZOffset);
    glm::mat4 postOffsetMat = createMatFromQuatAndPos(postOffsetRot, postOffsetTrans);

    _jointToPuckMap[controller::LEFT_HAND] = handPair.first;
    _pucksPostOffset[handPair.first] = postOffsetMat;
}

void ViveControllerManager::InputDevice::calibrateRightHand(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, PuckPosePair& handPair) {
    controller::Pose& handPose = handPair.second;
    glm::vec3 handPoseZAxis = handPose.getRotation() * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 referenceHandYAxis = transformVectorFast(defaultToReferenceMat * inputCalibration.defaultRightHand, glm::vec3(0.0f, 1.0f, 0.0f));
    const float EPSILON = 1.0e-4f;
    if (fabsf(fabsf(glm::dot(glm::normalize(referenceHandYAxis), glm::normalize(handPoseZAxis))) - 1.0f) < EPSILON) {
        handPoseZAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // This allows the user to not have to match the t-pose exactly.  We assume that the y facing of the hand lies in the plane of the puck.
    // Where the plane of the puck is defined by the the local z-axis of the puck, which is facing out of the vive logo/power button.
    glm::vec3 zPrime = handPoseZAxis;
    glm::vec3 xPrime = glm::normalize(glm::cross(referenceHandYAxis, handPoseZAxis));
    glm::vec3 yPrime = glm::normalize(glm::cross(zPrime, xPrime));
    glm::mat4 newHandMat = glm::mat4(glm::vec4(xPrime, 0.0f), glm::vec4(yPrime, 0.0f),
                                     glm::vec4(zPrime, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    glm::quat initialRot = handPose.getRotation();
    glm::quat postOffsetRot = glm::inverse(initialRot) * glmExtractRotation(newHandMat);
    glm::vec3 postOffsetTrans = postOffsetRot * -glm::vec3(0.0f, _handPuckYOffset, _handPuckZOffset);
    glm::mat4 postOffsetMat = createMatFromQuatAndPos(postOffsetRot, postOffsetTrans);

    _jointToPuckMap[controller::RIGHT_HAND] = handPair.first;
    _pucksPostOffset[handPair.first] = postOffsetMat;
}


void ViveControllerManager::InputDevice::calibrateFeet(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    glm::vec3 headXAxis = getReferenceHeadXAxis(defaultToReferenceMat, inputCalibration.defaultHeadMat);
    glm::vec3 headPosition = getReferenceHeadPosition(defaultToReferenceMat, inputCalibration.defaultHeadMat);
    auto& firstFoot = _validTrackedObjects[FIRST_FOOT];
    auto& secondFoot = _validTrackedObjects[SECOND_FOOT];
    controller::Pose& firstFootPose = firstFoot.second;
    controller::Pose& secondFootPose = secondFoot.second;

    if (determineLimbOrdering(firstFootPose, secondFootPose, headXAxis, headPosition)) {
        calibrateFoot(defaultToReferenceMat, inputCalibration, firstFoot, true);
        calibrateFoot(defaultToReferenceMat, inputCalibration, secondFoot, false);
    } else {
        calibrateFoot(defaultToReferenceMat, inputCalibration, secondFoot, true);
        calibrateFoot(defaultToReferenceMat, inputCalibration, firstFoot, false);
    }
}

void ViveControllerManager::InputDevice::calibrateFoot(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, PuckPosePair& footPair, bool isLeftFoot){
    controller::Pose footPose = footPair.second;
    glm::mat4 puckPoseMat = createMatFromQuatAndPos(footPose.getRotation(), footPose.getTranslation());
    glm::mat4 defaultFoot = isLeftFoot ? inputCalibration.defaultLeftFoot : inputCalibration.defaultRightFoot;
    glm::mat4 footOffset = computeOffset(defaultToReferenceMat, defaultFoot, footPose);
    glm::quat rotationOffset = glmExtractRotation(footOffset);
    glm::vec3 translationOffset = extractTranslation(footOffset);

    glm::vec3 localXAxisInPuckFrame = glm::normalize(transformVectorFast(glm::inverse(puckPoseMat) * defaultToReferenceMat, glm::vec3(-1.0f, 0.0f, 0.0f)));
    float distance = glm::dot(translationOffset, localXAxisInPuckFrame);

    // We ensure the offset vector lies in the sagittal plane of the avatar.
    // This helps prevent wide or narrow stances due to the user not matching the t-pose perfectly.
    glm::vec3 finalTranslation =  translationOffset - (distance * localXAxisInPuckFrame);
    glm::mat4 finalOffset = createMatFromQuatAndPos(rotationOffset, finalTranslation);

    if (isLeftFoot) {
        _jointToPuckMap[controller::LEFT_FOOT] = footPair.first;
        _pucksPostOffset[footPair.first] = finalOffset;
    } else {
        _jointToPuckMap[controller::RIGHT_FOOT] = footPair.first;
        _pucksPostOffset[footPair.first] = finalOffset;
    }
}

void ViveControllerManager::InputDevice::calibrateHips(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    _jointToPuckMap[controller::HIPS] = _validTrackedObjects[HIP].first;
    _pucksPostOffset[_validTrackedObjects[HIP].first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultHips, _validTrackedObjects[HIP].second);
}

void ViveControllerManager::InputDevice::calibrateChest(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    _jointToPuckMap[controller::SPINE2] = _validTrackedObjects[CHEST].first;
    _pucksPostOffset[_validTrackedObjects[CHEST].first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultSpine2, _validTrackedObjects[CHEST].second);
}

// y axis comes out of puck usb port/green light
// -z axis comes out of puck center/vive logo
static glm::vec3 computeUserShoulderPositionFromMeasurements(float armCirc, float shoulderSpan, const glm::mat4& headMat, const controller::Pose& armPuck, bool isLeftHand) {

    float armRadius = armCirc / TWO_PI;

    float sign = isLeftHand ? 1.0f : -1.0f;
    float localArmX = sign * shoulderSpan / 2.0f;

    controller::Pose localPuck = armPuck.transform(glm::inverse(headMat));
    glm::mat4 localPuckMat = localPuck.getMatrix();
    glm::vec3 localArmCenter = extractTranslation(localPuckMat) + armRadius * transformVectorFast(localPuckMat, Vectors::UNIT_Z);

    return transformPoint(headMat, glm::vec3(localArmX, localArmCenter.y, localArmCenter.z));
}

void ViveControllerManager::InputDevice::calibrateShoulders(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration,
                                                            int firstShoulderIndex, int secondShoulderIndex) {
    const PuckPosePair& firstShoulder = _validTrackedObjects[firstShoulderIndex];
    const PuckPosePair& secondShoulder = _validTrackedObjects[secondShoulderIndex];
    const controller::Pose& firstShoulderPose = firstShoulder.second;
    const controller::Pose& secondShoulderPose = secondShoulder.second;

    glm::mat4 refLeftArm = defaultToReferenceMat * inputCalibration.defaultLeftArm;
    glm::mat4 refRightArm = defaultToReferenceMat * inputCalibration.defaultRightArm;

    glm::mat4 userRefLeftArm = refLeftArm;
    glm::mat4 userRefRightArm = refRightArm;

    glm::mat4 headMat = defaultToReferenceMat * inputCalibration.defaultHeadMat;

    if (firstShoulderPose.translation.x < secondShoulderPose.translation.x) {
        _jointToPuckMap[controller::LEFT_ARM] = firstShoulder.first;
        _jointToPuckMap[controller::RIGHT_ARM] = secondShoulder.first;

        glm::vec3 leftPos = computeUserShoulderPositionFromMeasurements(_armCircumference, _shoulderWidth, headMat, firstShoulderPose, true);
        userRefLeftArm[3] = glm::vec4(leftPos, 1.0f);
        glm::vec3 rightPos = computeUserShoulderPositionFromMeasurements(_armCircumference, _shoulderWidth, headMat, secondShoulderPose, false);
        userRefRightArm[3] = glm::vec4(rightPos, 1.0f);

        // compute the post offset from the userRefArm
        _pucksPostOffset[firstShoulder.first] = computeOffset(Matrices::IDENTITY, userRefLeftArm, firstShoulderPose);
        _pucksPostOffset[secondShoulder.first] = computeOffset(Matrices::IDENTITY, userRefRightArm, secondShoulderPose);

        // compute the pre offset from the diff between userRefArm and refArm transforms.
        // as an optimization we don't do a full inverse, but subtract the translations.
        _pucksPreOffset[firstShoulder.first] = createMatFromQuatAndPos(glm::quat(), extractTranslation(userRefLeftArm) - extractTranslation(refLeftArm));
        _pucksPreOffset[secondShoulder.first] = createMatFromQuatAndPos(glm::quat(), extractTranslation(userRefRightArm) - extractTranslation(refRightArm));
    } else {
        _jointToPuckMap[controller::LEFT_ARM] = secondShoulder.first;
        _jointToPuckMap[controller::RIGHT_ARM] = firstShoulder.first;

        glm::vec3 leftPos = computeUserShoulderPositionFromMeasurements(_armCircumference, _shoulderWidth, headMat, secondShoulderPose, true);
        userRefLeftArm[3] = glm::vec4(leftPos, 1.0f);
        glm::vec3 rightPos = computeUserShoulderPositionFromMeasurements(_armCircumference, _shoulderWidth, headMat, firstShoulderPose, false);
        userRefRightArm[3] = glm::vec4(rightPos, 1.0f);

        // compute the post offset from the userRefArm
        _pucksPostOffset[secondShoulder.first] = computeOffset(Matrices::IDENTITY, userRefLeftArm, secondShoulderPose);
        _pucksPostOffset[firstShoulder.first] = computeOffset(Matrices::IDENTITY, userRefRightArm, firstShoulderPose);

        // compute the pre offset from the diff between userRefArm and refArm transforms.
        // as an optimization we don't do a full inverse, but subtract the translations.
        _pucksPreOffset[secondShoulder.first] = createMatFromQuatAndPos(glm::quat(), extractTranslation(userRefLeftArm) - extractTranslation(refLeftArm));
        _pucksPreOffset[firstShoulder.first] = createMatFromQuatAndPos(glm::quat(), extractTranslation(userRefRightArm) - extractTranslation(refRightArm));
    }
}

void ViveControllerManager::InputDevice::calibrateHead(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    size_t headIndex = _validTrackedObjects.size() - 1;
    const PuckPosePair& head = _validTrackedObjects[headIndex];
    _jointToPuckMap[controller::HEAD] = head.first;
    _pucksPostOffset[head.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultHeadMat, head.second);
}

QString ViveControllerManager::InputDevice::configToString(Config config) {
    return _configStringMap[config];
}

void ViveControllerManager::InputDevice::setConfigFromString(const QString& value) {
    if (value ==  "None") {
        _preferedConfig = Config::None;
    } else if (value == "Feet") {
        _preferedConfig = Config::Feet;
    } else if (value == "FeetAndHips") {
        _preferedConfig = Config::FeetAndHips;
    } else if (value == "FeetHipsAndChest") {
        _preferedConfig = Config::FeetHipsAndChest;
    } else if (value == "FeetHipsAndShoulders") {
        _preferedConfig = Config::FeetHipsAndShoulders;
    } else if (value == "FeetHipsChestAndShoulders") {
        _preferedConfig = Config::FeetHipsChestAndShoulders;
    }
}

/*@jsdoc
 * <p>The <code>Controller.Hardware.Vive</code> object has properties representing the Vive. The property values are integer 
 * IDs, uniquely identifying each output. <em>Read-only.</em></p>
 * <p>These outputs can be mapped to actions or functions or <code>Controller.Standard</code> items in a {@link RouteObject} 
 * mapping.</p>
 * <table>
 *   <thead>
 *     <tr><th>Property</th><th>Type</th><th>Data</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td colspan="4"><strong>Buttons</strong></td></tr>
 *     <tr><td><code>LeftApplicationMenu</code></td><td>number</td><td>number</td><td>Left application menu button pressed.
 *       </td></tr>
 *     <tr><td><code>RightApplicationMenu</code></td><td>number</td><td>number</td><td>Right application menu button pressed.
 *       </td></tr>
 *     <tr><td colspan="4"><strong>Touch Pad (Sticks)</strong></td></tr>
 *     <tr><td><code>LX</code></td><td>number</td><td>number</td><td>Left touch pad x-axis scale.</td></tr>
 *     <tr><td><code>LY</code></td><td>number</td><td>number</td><td>Left touch pad y-axis scale.</td></tr>
 *     <tr><td><code>RX</code></td><td>number</td><td>number</td><td>Right stick x-axis scale.</td></tr>
 *     <tr><td><code>RY</code></td><td>number</td><td>number</td><td>Right stick y-axis scale.</td></tr>
 *     <tr><td><code>LS</code></td><td>number</td><td>number</td><td>Left touch pad pressed.</td></tr>
 *     <tr><td><code>LSCenter</code></td><td>number</td><td>number</td><td>Left touch pad center pressed.</td></tr>
 *     <tr><td><code>LSX</code></td><td>number</td><td>number</td><td>Left touch pad pressed x-coordinate.</td></tr>
 *     <tr><td><code>LSY</code></td><td>number</td><td>number</td><td>Left touch pad pressed y-coordinate.</td></tr>
 *     <tr><td><code>RS</code></td><td>number</td><td>number</td><td>Right touch pad pressed.</td></tr>
 *     <tr><td><code>RSCenter</code></td><td>number</td><td>number</td><td>Right touch pad center pressed.</td></tr>
 *     <tr><td><code>RSX</code></td><td>number</td><td>number</td><td>Right touch pad pressed x-coordinate.</td></tr>
 *     <tr><td><code>RSY</code></td><td>number</td><td>number</td><td>Right touch pad pressed y-coordinate.</td></tr>
 *     <tr><td><code>LSTouch</code></td><td>number</td><td>number</td><td>Left touch pad is touched.</td></tr>
 *     <tr><td><code>RSTouch</code></td><td>number</td><td>number</td><td>Right touch pad is touched.</td></tr>
 *     <tr><td colspan="4"><strong>Triggers</strong></td></tr>
 *     <tr><td><code>LT</code></td><td>number</td><td>number</td><td>Left trigger scale.</td></tr>
 *     <tr><td><code>RT</code></td><td>number</td><td>number</td><td>Right trigger scale.</td></tr>
 *     <tr><td><code>LTClick</code></td><td>number</td><td>number</td><td>Left trigger click.</td></tr>
 *     <tr><td><code>RTClick</code></td><td>number</td><td>number</td><td>Right trigger click.</td></tr>
 *     <tr><td><code>LeftGrip</code></td><td>number</td><td>number</td><td>Left grip scale.</td></tr>
 *     <tr><td><code>RightGrip</code></td><td>number</td><td>number</td><td>Right grip scale.</td></tr>
 *     <tr><td colspan="4"><strong>Avatar Skeleton</strong></td></tr>
 *     <tr><td><code>Hips</code></td><td>number</td><td>{@link Pose}</td><td>Hips pose.</td></tr>
 *     <tr><td><code>Spine2</code></td><td>number</td><td>{@link Pose}</td><td>Spine2 pose.</td></tr>
 *     <tr><td><code>Head</code></td><td>number</td><td>{@link Pose}</td><td>Head pose.</td></tr>
 *     <tr><td><code>LeftEye</code></td><td>number</td><td>{@link Pose}</td><td>Left eye pose.</td></tr>
 *     <tr><td><code>RightEye</code></td><td>number</td><td>{@link Pose}</td><td>Right eye pose.</td></tr>
 *     <tr><td><code>EyeBlink_L</code></td><td>number</td><td>number</td><td>Left eyelid blink.</td></tr>
 *     <tr><td><code>EyeBlink_R</code></td><td>number</td><td>number</td><td>Right eyelid blink.</td></tr>
 *     <tr><td><code>LeftArm</code></td><td>number</td><td>{@link Pose}</td><td>Left arm pose.</td></tr>
 *     <tr><td><code>RightArm</code></td><td>number</td><td>{@link Pose}</td><td>Right arm pose</td></tr>
 *     <tr><td><code>LeftHand</code></td><td>number</td><td>{@link Pose}</td><td>Left hand pose.</td></tr>
 *     <tr><td><code>LeftHandThumb1</code></td><td>number</td><td>{@link Pose}</td><td>Left thumb 1 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandThumb2</code></td><td>number</td><td>{@link Pose}</td><td>Left thumb 2 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandThumb3</code></td><td>number</td><td>{@link Pose}</td><td>Left thumb 3 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandThumb4</code></td><td>number</td><td>{@link Pose}</td><td>Left thumb 4 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandIndex1</code></td><td>number</td><td>{@link Pose}</td><td>Left index 1 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandIndex2</code></td><td>number</td><td>{@link Pose}</td><td>Left index 2 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandIndex3</code></td><td>number</td><td>{@link Pose}</td><td>Left index 3 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandIndex4</code></td><td>number</td><td>{@link Pose}</td><td>Left index 4 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandMiddle1</code></td><td>number</td><td>{@link Pose}</td><td>Left middle 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftHandMiddle2</code></td><td>number</td><td>{@link Pose}</td><td>Left middle 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftHandMiddle3</code></td><td>number</td><td>{@link Pose}</td><td>Left middle 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftHandMiddle4</code></td><td>number</td><td>{@link Pose}</td><td>Left middle 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftHandRing1</code></td><td>number</td><td>{@link Pose}</td><td>Left ring 1 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandRing2</code></td><td>number</td><td>{@link Pose}</td><td>Left ring 2 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandRing3</code></td><td>number</td><td>{@link Pose}</td><td>Left ring 3 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandRing4</code></td><td>number</td><td>{@link Pose}</td><td>Left ring 4 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandPinky1</code></td><td>number</td><td>{@link Pose}</td><td>Left pinky 1 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandPinky2</code></td><td>number</td><td>{@link Pose}</td><td>Left pinky 2 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandPinky3</code></td><td>number</td><td>{@link Pose}</td><td>Left pinky 3 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandPinky4</code></td><td>number</td><td>{@link Pose}</td><td>Left pinky 4 finger joint pose.</td></tr>
 *     <tr><td><code>RightHand</code></td><td>number</td><td>{@link Pose}</td><td>Right hand pose.</td></tr>
 *     <tr><td><code>RightHandThumb1</code></td><td>number</td><td>{@link Pose}</td><td>Right thumb 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandThumb2</code></td><td>number</td><td>{@link Pose}</td><td>Right thumb 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandThumb3</code></td><td>number</td><td>{@link Pose}</td><td>Right thumb 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandThumb4</code></td><td>number</td><td>{@link Pose}</td><td>Right thumb 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandIndex1</code></td><td>number</td><td>{@link Pose}</td><td>Right index 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandIndex2</code></td><td>number</td><td>{@link Pose}</td><td>Right index 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandIndex3</code></td><td>number</td><td>{@link Pose}</td><td>Right index 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandIndex4</code></td><td>number</td><td>{@link Pose}</td><td>Right index 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandMiddle1</code></td><td>number</td><td>{@link Pose}</td><td>Right middle 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandMiddle2</code></td><td>number</td><td>{@link Pose}</td><td>Right middle 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandMiddle3</code></td><td>number</td><td>{@link Pose}</td><td>Right middle 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandMiddle4</code></td><td>number</td><td>{@link Pose}</td><td>Right middle 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandRing1</code></td><td>number</td><td>{@link Pose}</td><td>Right ring 1 finger joint pose.</td></tr>
 *     <tr><td><code>RightHandRing2</code></td><td>number</td><td>{@link Pose}</td><td>Right ring 2 finger joint pose.</td></tr>
 *     <tr><td><code>RightHandRing3</code></td><td>number</td><td>{@link Pose}</td><td>Right ring 3 finger joint pose.</td></tr>
 *     <tr><td><code>RightHandRing4</code></td><td>number</td><td>{@link Pose}</td><td>Right ring 4 finger joint pose.</td></tr>
 *     <tr><td><code>RightHandPinky1</code></td><td>number</td><td>{@link Pose}</td><td>Right pinky 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandPinky2</code></td><td>number</td><td>{@link Pose}</td><td>Right pinky 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandPinky3</code></td><td>number</td><td>{@link Pose}</td><td>Right pinky 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandPinky4</code></td><td>number</td><td>{@link Pose}</td><td>Right pinky 4 finger joint pose.
 *       </td></tr>
 *     <tr><td colspan="4"><strong>Trackers</strong></td></tr>
 *     <tr><td><code>TrackedObject00</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 0 pose.</td></tr>
 *     <tr><td><code>TrackedObject01</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 1 pose.</td></tr>
 *     <tr><td><code>TrackedObject02</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 2 pose.</td></tr>
 *     <tr><td><code>TrackedObject03</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 3 pose.</td></tr>
 *     <tr><td><code>TrackedObject04</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 4 pose.</td></tr>
 *     <tr><td><code>TrackedObject05</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 5 pose.</td></tr>
 *     <tr><td><code>TrackedObject06</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 6 pose.</td></tr>
 *     <tr><td><code>TrackedObject07</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 7 pose.</td></tr>
 *     <tr><td><code>TrackedObject08</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 8 pose.</td></tr>
 *     <tr><td><code>TrackedObject09</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 9 pose.</td></tr>
 *     <tr><td><code>TrackedObject10</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 10 pose.</td></tr>
 *     <tr><td><code>TrackedObject11</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 11 pose.</td></tr>
 *     <tr><td><code>TrackedObject12</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 12 pose.</td></tr>
 *     <tr><td><code>TrackedObject13</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 13 pose.</td></tr>
 *     <tr><td><code>TrackedObject14</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 14 pose.</td></tr>
 *     <tr><td><code>TrackedObject15</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 15 pose.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {object} Controller.Hardware-Vive
 */
controller::Input::NamedVector ViveControllerManager::InputDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs{
        // Trackpad analogs
        makePair(LX, "LX"),
        makePair(LY, "LY"),
        makePair(RX, "RX"),
        makePair(RY, "RY"),

        // capacitive touch on the touch pad
        makePair(LS_TOUCH, "LSTouch"),
        makePair(RS_TOUCH, "RSTouch"),

        // touch pad press
        makePair(LS, "LS"),
        makePair(RS, "RS"),
        // Differentiate where we are in the touch pad click
        makePair(LS_CENTER, "LSCenter"),
        makePair(LS_X, "LSX"),
        makePair(LS_Y, "LSY"),
        makePair(RS_CENTER, "RSCenter"),
        makePair(RS_X, "RSX"),
        makePair(RS_Y, "RSY"),


        // triggers
        makePair(LT, "LT"),
        makePair(RT, "RT"),

        // Trigger clicks
        makePair(LT_CLICK, "LTClick"),
        makePair(RT_CLICK, "RTClick"),

        // low profile side grip button.
        makePair(LEFT_GRIP, "LeftGrip"),
        makePair(RIGHT_GRIP, "RightGrip"),

        // 3d location of left controller and fingers
        makePair(LEFT_HAND, "LeftHand"),
        makePair(LEFT_HAND_THUMB1, "LeftHandThumb1"),
        makePair(LEFT_HAND_THUMB2, "LeftHandThumb2"),
        makePair(LEFT_HAND_THUMB3, "LeftHandThumb3"),
        makePair(LEFT_HAND_THUMB4, "LeftHandThumb4"),
        makePair(LEFT_HAND_INDEX1, "LeftHandIndex1"),
        makePair(LEFT_HAND_INDEX2, "LeftHandIndex2"),
        makePair(LEFT_HAND_INDEX3, "LeftHandIndex3"),
        makePair(LEFT_HAND_INDEX4, "LeftHandIndex4"),
        makePair(LEFT_HAND_MIDDLE1, "LeftHandMiddle1"),
        makePair(LEFT_HAND_MIDDLE2, "LeftHandMiddle2"),
        makePair(LEFT_HAND_MIDDLE3, "LeftHandMiddle3"),
        makePair(LEFT_HAND_MIDDLE4, "LeftHandMiddle4"),
        makePair(LEFT_HAND_RING1, "LeftHandRing1"),
        makePair(LEFT_HAND_RING2, "LeftHandRing2"),
        makePair(LEFT_HAND_RING3, "LeftHandRing3"),
        makePair(LEFT_HAND_RING4, "LeftHandRing4"),
        makePair(LEFT_HAND_PINKY1, "LeftHandPinky1"),
        makePair(LEFT_HAND_PINKY2, "LeftHandPinky2"),
        makePair(LEFT_HAND_PINKY3, "LeftHandPinky3"),
        makePair(LEFT_HAND_PINKY4, "LeftHandPinky4"),

        // 3d location of right controller and fingers
        makePair(RIGHT_HAND, "RightHand"),
        makePair(RIGHT_HAND_THUMB1, "RightHandThumb1"),
        makePair(RIGHT_HAND_THUMB2, "RightHandThumb2"),
        makePair(RIGHT_HAND_THUMB3, "RightHandThumb3"),
        makePair(RIGHT_HAND_THUMB4, "RightHandThumb4"),
        makePair(RIGHT_HAND_INDEX1, "RightHandIndex1"),
        makePair(RIGHT_HAND_INDEX2, "RightHandIndex2"),
        makePair(RIGHT_HAND_INDEX3, "RightHandIndex3"),
        makePair(RIGHT_HAND_INDEX4, "RightHandIndex4"),
        makePair(RIGHT_HAND_MIDDLE1, "RightHandMiddle1"),
        makePair(RIGHT_HAND_MIDDLE2, "RightHandMiddle2"),
        makePair(RIGHT_HAND_MIDDLE3, "RightHandMiddle3"),
        makePair(RIGHT_HAND_MIDDLE4, "RightHandMiddle4"),
        makePair(RIGHT_HAND_RING1, "RightHandRing1"),
        makePair(RIGHT_HAND_RING2, "RightHandRing2"),
        makePair(RIGHT_HAND_RING3, "RightHandRing3"),
        makePair(RIGHT_HAND_RING4, "RightHandRing4"),
        makePair(RIGHT_HAND_PINKY1, "RightHandPinky1"),
        makePair(RIGHT_HAND_PINKY2, "RightHandPinky2"),
        makePair(RIGHT_HAND_PINKY3, "RightHandPinky3"),
        makePair(RIGHT_HAND_PINKY4, "RightHandPinky4"),

        makePair(LEFT_FOOT, "LeftFoot"),
        makePair(RIGHT_FOOT, "RightFoot"),
        makePair(HIPS, "Hips"),
        makePair(SPINE2, "Spine2"),
        makePair(HEAD, "Head"),
        makePair(LEFT_ARM, "LeftArm"),
        makePair(RIGHT_ARM, "RightArm"),
        makePair(LEFT_EYE, "LeftEye"),
        makePair(RIGHT_EYE, "RightEye"),
        makePair(EYEBLINK_L, "EyeBlink_L"),
        makePair(EYEBLINK_R, "EyeBlink_R"),

        // 16 tracked poses
        makePair(TRACKED_OBJECT_00, "TrackedObject00"),
        makePair(TRACKED_OBJECT_01, "TrackedObject01"),
        makePair(TRACKED_OBJECT_02, "TrackedObject02"),
        makePair(TRACKED_OBJECT_03, "TrackedObject03"),
        makePair(TRACKED_OBJECT_04, "TrackedObject04"),
        makePair(TRACKED_OBJECT_05, "TrackedObject05"),
        makePair(TRACKED_OBJECT_06, "TrackedObject06"),
        makePair(TRACKED_OBJECT_07, "TrackedObject07"),
        makePair(TRACKED_OBJECT_08, "TrackedObject08"),
        makePair(TRACKED_OBJECT_09, "TrackedObject09"),
        makePair(TRACKED_OBJECT_10, "TrackedObject10"),
        makePair(TRACKED_OBJECT_11, "TrackedObject11"),
        makePair(TRACKED_OBJECT_12, "TrackedObject12"),
        makePair(TRACKED_OBJECT_13, "TrackedObject13"),
        makePair(TRACKED_OBJECT_14, "TrackedObject14"),
        makePair(TRACKED_OBJECT_15, "TrackedObject15"),

        // app button above trackpad.
        Input::NamedPair(Input(_deviceID, LEFT_APP_MENU, ChannelType::BUTTON), "LeftApplicationMenu"),
        Input::NamedPair(Input(_deviceID, RIGHT_APP_MENU, ChannelType::BUTTON), "RightApplicationMenu"),
    };

    return availableInputs;
}

QString ViveControllerManager::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/vive.json";
    return MAPPING_JSON;
}
