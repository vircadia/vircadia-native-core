//
//  ViveControllerManager.cpp
//  input-plugins/src/input-plugins
//
//  Created by Sam Gondelman on 6/29/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ViveControllerManager.h"
#include <algorithm>

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
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>

#include <controllers/UserInputMapper.h>

#include <controllers/StandardControls.h>


extern PoseData _nextSimPoseData;

vr::IVRSystem* acquireOpenVrSystem();
void releaseOpenVrSystem();


static const char* CONTROLLER_MODEL_STRING = "vr_controller_05_wireless_b";
const quint64 CALIBRATION_TIMELAPSE = 1 * USECS_PER_SECOND;

static const char* MENU_PARENT = "Avatar";
static const char* MENU_NAME = "Vive Controllers";
static const char* MENU_PATH = "Avatar" ">" "Vive Controllers";
static const char* RENDER_CONTROLLERS = "Render Hand Controllers";
static const int MIN_PUCK_COUNT = 2;
static const int MIN_FEET_AND_HIPS = 3;
static const int MIN_FEET_HIPS_CHEST = 4;
static const int MIN_FEET_HIPS_HEAD = 4;
static const int MIN_FEET_HIPS_SHOULDERS = 5;
static const int MIN_FEET_HIPS_CHEST_HEAD = 5;
static const int FIRST_FOOT = 0;
static const int SECOND_FOOT = 1;
static const int HIP = 2;
static const int CHEST = 3;
static float HEAD_PUCK_Y_OFFSET = -0.0254f;
static float HEAD_PUCK_Z_OFFSET = -0.152f;

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

static bool sortPucksYPosition(PuckPosePair firstPuck, PuckPosePair secondPuck) {
    return (firstPuck.second.translation.y < secondPuck.second.translation.y);
}

static bool sortPucksXPosition(PuckPosePair firstPuck, PuckPosePair secondPuck) {
    return (firstPuck.second.translation.x < secondPuck.second.translation.x);
}

static bool determineFeetOrdering(const controller::Pose& poseA, const controller::Pose& poseB, glm::vec3 axis, glm::vec3 axisOrigin) {
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

bool ViveControllerManager::isSupported() const {
    return openVrSupported();
}

bool ViveControllerManager::activate() {
    InputPlugin::activate();

    _container->addMenu(MENU_PATH);
    _container->addMenuItem(PluginType::INPUT_PLUGIN, MENU_PATH, RENDER_CONTROLLERS,
        [this] (bool clicked) { this->setRenderControllers(clicked); },
        true, true);

    if (!_system) {
        _system = acquireOpenVrSystem();
    }
    Q_ASSERT(_system);

    enableOpenVrKeyboard(_container);

    // register with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(_inputDevice);
    _registeredWithInputMapper = true;
    return true;
}

void ViveControllerManager::deactivate() {
    InputPlugin::deactivate();

    disableOpenVrKeyboard();

    _container->removeMenuItem(MENU_NAME, RENDER_CONTROLLERS);
    _container->removeMenu(MENU_PATH);

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
}

bool ViveControllerManager::isHeadControllerMounted() const {
    if (_inputDevice && _inputDevice->isHeadControllerMounted()) {
        return true;
    }
    vr::EDeviceActivityLevel activityLevel = _system->GetTrackedDeviceActivityLevel(vr::k_unTrackedDeviceIndex_Hmd);
    return activityLevel == vr::k_EDeviceActivityLevel_UserInteraction;
}

void ViveControllerManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {

    if (!_system) {
        return;
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
}

ViveControllerManager::InputDevice::InputDevice(vr::IVRSystem*& system) : controller::InputDevice("Vive"), _system(system) {

    _configStringMap[Config::Auto] =  QString("Auto");
    _configStringMap[Config::Feet] =  QString("Feet");
    _configStringMap[Config::FeetAndHips] =  QString("FeetAndHips");
    _configStringMap[Config::FeetHipsAndChest] =  QString("FeetHipsAndChest");
    _configStringMap[Config::FeetHipsAndShoulders] = QString("FeetHipsAndShoulders");
    _configStringMap[Config::FeetHipsChestAndHead] = QString("FeetHipsChestAndHead");
    _configStringMap[Config::FeetHipsAndHead] = QString("FeetHipsAndHead");

    if (openVrSupported()) {
        createPreferences();
    }
}

void ViveControllerManager::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _poseStateMap.clear();
    _buttonPressedMap.clear();
    _validTrackedObjects.clear();

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
    for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; i++) {
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

    int numTrackedControllers = 0;
    if (leftHandDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
        numTrackedControllers++;
    }
    if (rightHandDeviceIndex != vr::k_unTrackedDeviceIndexInvalid) {
        numTrackedControllers++;
    }
    _trackedControllers = numTrackedControllers;

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

    updateCalibratedLimbs();
    _lastSimPoseData = _nextSimPoseData;
}

void ViveControllerManager::InputDevice::handleTrackedObject(uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData) {
    uint32_t poseIndex = controller::TRACKED_OBJECT_00 + deviceIndex;
    printDeviceTrackingResultChange(deviceIndex);
    if (_system->IsTrackedDeviceConnected(deviceIndex) &&
        _system->GetTrackedDeviceClass(deviceIndex) == vr::TrackedDeviceClass_GenericTracker &&
        _nextSimPoseData.vrPoses[deviceIndex].bPoseIsValid &&
        poseIndex <= controller::TRACKED_OBJECT_15) {

        mat4& mat = mat4();
        vec3 linearVelocity = vec3();
        vec3 angularVelocity = vec3();
        // check if the device is tracking out of range, then process the correct pose depending on the result.
        if (_nextSimPoseData.vrPoses[deviceIndex].eTrackingResult != vr::TrackingResult_Running_OutOfRange) {
            mat = _nextSimPoseData.poses[deviceIndex];
            linearVelocity = _nextSimPoseData.linearVelocities[deviceIndex];
            angularVelocity = _nextSimPoseData.angularVelocities[deviceIndex];
        } else {
            mat = _lastSimPoseData.poses[deviceIndex];
            linearVelocity = _lastSimPoseData.linearVelocities[deviceIndex];
            angularVelocity = _lastSimPoseData.angularVelocities[deviceIndex];

            // make sure that we do not overwrite the pose in the _lastSimPose with incorrect data.
            _nextSimPoseData.poses[deviceIndex] = _lastSimPoseData.poses[deviceIndex];
            _nextSimPoseData.linearVelocities[deviceIndex] = _lastSimPoseData.linearVelocities[deviceIndex];
            _nextSimPoseData.angularVelocities[deviceIndex] = _lastSimPoseData.angularVelocities[deviceIndex];

        }

        controller::Pose pose(extractTranslation(mat), glmExtractRotation(mat), linearVelocity, angularVelocity);

        // transform into avatar frame
        glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
        _poseStateMap[poseIndex] = pose.transform(controllerToAvatar);
        _validTrackedObjects.push_back(std::make_pair(poseIndex, _poseStateMap[poseIndex]));
    } else {
        controller::Pose invalidPose;
        _poseStateMap[poseIndex] = invalidPose;
    }
}

void ViveControllerManager::InputDevice::calibrateOrUncalibrate(const controller::InputCalibrationData& inputCalibration) {
    if (!_calibrated) {
        calibrate(inputCalibration);
    } else {
        uncalibrate();
    }
}

void ViveControllerManager::InputDevice::calibrate(const controller::InputCalibrationData& inputCalibration) {
    qDebug() << "Puck Calibration: Starting...";
    // convert the hmd head from sensor space to avatar space
    glm::mat4 hmdSensorFlippedMat = inputCalibration.hmdSensorMat * Matrices::Y_180;
    glm::mat4 sensorToAvatarMat = glm::inverse(inputCalibration.avatarMat) * inputCalibration.sensorToWorldMat;
    glm::mat4 hmdAvatarMat = sensorToAvatarMat * hmdSensorFlippedMat;

    // cancel the roll and pitch for the hmd head
    glm::quat hmdRotation = cancelOutRollAndPitch(glmExtractRotation(hmdAvatarMat));
    glm::vec3 hmdTranslation = extractTranslation(hmdAvatarMat);
    glm::mat4 currentHmd = createMatFromQuatAndPos(hmdRotation, hmdTranslation);

    // calculate the offset from the centerOfEye to defaultHeadMat
    glm::mat4 defaultHeadOffset = glm::inverse(inputCalibration.defaultCenterEyeMat) * inputCalibration.defaultHeadMat;

    glm::mat4 currentHead  = currentHmd * defaultHeadOffset;

    // calculate the defaultToRefrenceXform
    glm::mat4 defaultToReferenceMat = currentHead * glm::inverse(inputCalibration.defaultHeadMat);

    int puckCount = (int)_validTrackedObjects.size();
    qDebug() << "Puck Calibration: " << puckCount << " pucks found for calibration";
    _config = _preferedConfig;
    if (_config != Config::Auto && puckCount < MIN_PUCK_COUNT) {
        qDebug() << "Puck Calibration: Failed: Could not meet the minimal # of pucks";
        uncalibrate();
        return;
    } else if (_config == Config::Auto){
        if (puckCount == MIN_PUCK_COUNT) {
            _config = Config::Feet;
            qDebug() << "Puck Calibration: Auto Config: " << configToString(_config) << " configuration";
        } else if (puckCount == MIN_FEET_AND_HIPS) {
            _config = Config::FeetAndHips;
            qDebug() << "Puck Calibration: Auto Config: " << configToString(_config) << " configuration";
        } else if (puckCount >= MIN_FEET_HIPS_CHEST) {
            _config = Config::FeetHipsAndChest;
            qDebug() << "Puck Calibration: Auto Config: " << configToString(_config) << " configuration";
        } else {
            qDebug() << "Puck Calibration: Auto Config Failed: Could not meet the minimal # of pucks";
            uncalibrate();
            return;
        }
    }

    std::sort(_validTrackedObjects.begin(), _validTrackedObjects.end(), sortPucksYPosition);

    if (_config == Config::Feet) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
    } else if (_config == Config::FeetAndHips && puckCount >= MIN_FEET_AND_HIPS) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
        calibrateHips(defaultToReferenceMat, inputCalibration);
    } else if (_config == Config::FeetHipsAndChest && puckCount >= MIN_FEET_HIPS_CHEST) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
        calibrateHips(defaultToReferenceMat, inputCalibration);
        calibrateChest(defaultToReferenceMat, inputCalibration);
    } else if (_config == Config::FeetHipsAndShoulders && puckCount >= MIN_FEET_HIPS_SHOULDERS) {
        calibrateFeet(defaultToReferenceMat, inputCalibration);
        calibrateHips(defaultToReferenceMat, inputCalibration);
        int firstShoulderIndex = 3;
        int secondShoulderIndex = 4;
        calibrateShoulders(defaultToReferenceMat, inputCalibration, firstShoulderIndex, secondShoulderIndex);
    } else if (_config == Config::FeetHipsAndHead && puckCount == MIN_FEET_HIPS_HEAD) {
        glm::mat4 headPuckDefaultToReferenceMat = recalculateDefaultToReferenceForHeadPuck(inputCalibration);
        glm::vec3 headXAxis = getReferenceHeadXAxis(headPuckDefaultToReferenceMat, inputCalibration.defaultHeadMat);
        glm::vec3 headPosition = getReferenceHeadPosition(headPuckDefaultToReferenceMat, inputCalibration.defaultHeadMat);
        calibrateFeet(headPuckDefaultToReferenceMat, inputCalibration, headXAxis, headPosition);
        calibrateHips(headPuckDefaultToReferenceMat, inputCalibration);
        calibrateHead(headPuckDefaultToReferenceMat, inputCalibration);
        _overrideHead = true;
    } else if (_config == Config::FeetHipsChestAndHead && puckCount == MIN_FEET_HIPS_CHEST_HEAD) {
        glm::mat4 headPuckDefaultToReferenceMat = recalculateDefaultToReferenceForHeadPuck(inputCalibration);
        glm::vec3 headXAxis = getReferenceHeadXAxis(headPuckDefaultToReferenceMat, inputCalibration.defaultHeadMat);
        glm::vec3 headPosition = getReferenceHeadPosition(headPuckDefaultToReferenceMat, inputCalibration.defaultHeadMat);
        calibrateFeet(headPuckDefaultToReferenceMat, inputCalibration, headXAxis, headPosition);
        calibrateHips(headPuckDefaultToReferenceMat, inputCalibration);
        calibrateChest(headPuckDefaultToReferenceMat, inputCalibration);
        calibrateHead(headPuckDefaultToReferenceMat, inputCalibration);
        _overrideHead = true;
    } else {
        qDebug() << "Puck Calibration: " << configToString(_config) << " Config Failed: Could not meet the minimal # of pucks";
        uncalibrate();
        return;
    }
    _calibrated = true;
    qDebug() << "PuckCalibration: " << configToString(_config) << " Configuration Successful";
}

void ViveControllerManager::InputDevice::uncalibrate() {
    _config = Config::Auto;
    _pucksOffset.clear();
    _jointToPuckMap.clear();
    _calibrated = false;
    _overrideHead = false;
}

void ViveControllerManager::InputDevice::updateCalibratedLimbs() {
    _poseStateMap[controller::LEFT_FOOT] = addOffsetToPuckPose(controller::LEFT_FOOT);
    _poseStateMap[controller::RIGHT_FOOT] = addOffsetToPuckPose(controller::RIGHT_FOOT);
    _poseStateMap[controller::HIPS] = addOffsetToPuckPose(controller::HIPS);
    _poseStateMap[controller::SPINE2] = addOffsetToPuckPose(controller::SPINE2);
    _poseStateMap[controller::RIGHT_ARM] = addOffsetToPuckPose(controller::RIGHT_ARM);
    _poseStateMap[controller::LEFT_ARM] = addOffsetToPuckPose(controller::LEFT_ARM);

    if (_overrideHead) {
        _poseStateMap[controller::HEAD] = addOffsetToPuckPose(controller::HEAD);
    }
}

controller::Pose ViveControllerManager::InputDevice::addOffsetToPuckPose(int joint) const {
    auto puck = _jointToPuckMap.find(joint);
    if (puck != _jointToPuckMap.end()) {
        uint32_t puckIndex = puck->second;
        auto puckPose = _poseStateMap.find(puckIndex);
        auto puckOffset = _pucksOffset.find(puckIndex);

        if ((puckPose != _poseStateMap.end()) && (puckOffset != _pucksOffset.end())) {
            return puckPose->second.postTransform(puckOffset->second);
        }
    }
    return controller::Pose();
}

void ViveControllerManager::InputDevice::handleHmd(uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData) {
     uint32_t poseIndex = controller::TRACKED_OBJECT_00 + deviceIndex;

     if (_system->IsTrackedDeviceConnected(deviceIndex) &&
         _system->GetTrackedDeviceClass(deviceIndex) == vr::TrackedDeviceClass_HMD &&
         _nextSimPoseData.vrPoses[deviceIndex].bPoseIsValid) {

         const mat4& mat = _nextSimPoseData.poses[deviceIndex];
         const vec3 linearVelocity = _nextSimPoseData.linearVelocities[deviceIndex];
         const vec3 angularVelocity = _nextSimPoseData.angularVelocities[deviceIndex];

         handleHeadPoseEvent(inputCalibrationData, mat, linearVelocity, angularVelocity);
     }
}

void ViveControllerManager::InputDevice::handleHandController(float deltaTime, uint32_t deviceIndex, const controller::InputCalibrationData& inputCalibrationData, bool isLeftHand) {

    if (_system->IsTrackedDeviceConnected(deviceIndex) &&
        _system->GetTrackedDeviceClass(deviceIndex) == vr::TrackedDeviceClass_Controller &&
        _nextSimPoseData.vrPoses[deviceIndex].bPoseIsValid) {

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

glm::mat4 ViveControllerManager::InputDevice::recalculateDefaultToReferenceForHeadPuck(const controller::InputCalibrationData& inputCalibration) {
    glm::mat4 avatarToSensorMat = glm::inverse(inputCalibration.sensorToWorldMat) * inputCalibration.avatarMat;
    glm::mat4 sensorToAvatarMat = glm::inverse(inputCalibration.avatarMat) * inputCalibration.sensorToWorldMat;
    size_t headPuckIndex = _validTrackedObjects.size() - 1;
    controller::Pose headPuckPose = _validTrackedObjects[headPuckIndex].second;
    glm::mat4 headPuckAvatarMat = createMatFromQuatAndPos(headPuckPose.getRotation(), headPuckPose.getTranslation()) * Matrices::Y_180;
    glm::vec3 headPuckTranslation = extractTranslation(headPuckAvatarMat);
    glm::vec3 headPuckZAxis = cancelOutRollAndPitch(glmExtractRotation(headPuckAvatarMat)) * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // check that the head puck z axis is not parrallel to the world up
    const float EPSILON = 1.0e-4f;
    glm::vec3 zAxis = glmExtractRotation(headPuckAvatarMat) * glm::vec3(0.0f, 0.0f, 1.0f);
    if (fabsf(fabsf(glm::dot(glm::normalize(worldUp), glm::normalize(zAxis))) - 1.0f) < EPSILON) {
        headPuckZAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 yPrime = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 xPrime = glm::normalize(glm::cross(worldUp, headPuckZAxis));
    glm::vec3 zPrime = glm::normalize(glm::cross(xPrime, yPrime));
    glm::mat4 newHeadPuck = glm::mat4(glm::vec4(xPrime, 0.0f), glm::vec4(yPrime, 0.0f),
                                      glm::vec4(zPrime, 0.0f), glm::vec4(headPuckTranslation, 1.0f));

    glm::mat4 headPuckOffset = glm::mat4(glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                                         glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, HEAD_PUCK_Y_OFFSET, HEAD_PUCK_Z_OFFSET, 1.0f));

    glm::mat4 finalHeadPuck = newHeadPuck * headPuckOffset;

    glm::mat4 defaultHeadOffset = glm::inverse(inputCalibration.defaultCenterEyeMat) * inputCalibration.defaultHeadMat;

    glm::mat4 currentHead  = finalHeadPuck * defaultHeadOffset;

    // calculate the defaultToRefrenceXform
    glm::mat4 defaultToReferenceMat = currentHead * glm::inverse(inputCalibration.defaultHeadMat);
    return defaultToReferenceMat;
}

void ViveControllerManager::InputDevice::partitionTouchpad(int sButton, int xAxis, int yAxis, int centerPseudoButton, int xPseudoButton, int yPseudoButton) {
    // Populate the L/RS_CENTER/OUTER pseudo buttons, corresponding to a partition of the L/RS space based on the X/Y values.
    const float CENTER_DEADBAND = 0.6f;
    const float DIAGONAL_DIVIDE_IN_RADIANS = PI / 4.0f;
    if (_buttonPressedMap.find(sButton) != _buttonPressedMap.end()) {
        float absX = abs(_axisStateMap[xAxis]);
        float absY = abs(_axisStateMap[yAxis]);
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
        _axisStateMap[isLeftHand ? LX : RX] = stick.x;
        _axisStateMap[isLeftHand ? LY : RY] = stick.y;
    } else if (axis == vr::k_EButton_SteamVR_Trigger) {
        _axisStateMap[isLeftHand ? LT : RT] = x;
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
    auto& endOfMap = _buttonPressedMap.end();
    auto& leftTrigger = _buttonPressedMap.find(controller::LT);
    auto& rightTrigger = _buttonPressedMap.find(controller::RT);
    auto& leftAppButton = _buttonPressedMap.find(LEFT_APP_MENU);
    auto& rightAppButton = _buttonPressedMap.find(RIGHT_APP_MENU);
    return ((leftTrigger != endOfMap && leftAppButton != endOfMap) && (rightTrigger != endOfMap && rightAppButton != endOfMap));
}

// These functions do translation from the Steam IDs to the standard controller IDs
void ViveControllerManager::InputDevice::handleButtonEvent(float deltaTime, uint32_t button, bool pressed, bool touched, bool isLeftHand) {

    using namespace controller;

    if (pressed) {
        if (button == vr::k_EButton_ApplicationMenu) {
            _buttonPressedMap.insert(isLeftHand ? LEFT_APP_MENU : RIGHT_APP_MENU);
        } else if (button == vr::k_EButton_Grip) {
            _axisStateMap[isLeftHand ? LEFT_GRIP : RIGHT_GRIP] = 1.0f;
        } else if (button == vr::k_EButton_SteamVR_Trigger) {
            _buttonPressedMap.insert(isLeftHand ? LT : RT);
        } else if (button == vr::k_EButton_SteamVR_Touchpad) {
            _buttonPressedMap.insert(isLeftHand ? LS : RS);
        }
    } else {
        if (button == vr::k_EButton_Grip) {
            _axisStateMap[isLeftHand ? LEFT_GRIP : RIGHT_GRIP] = 0.0f;
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

    glm::mat4 sensorToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    glm::mat4 defaultHeadOffset = glm::inverse(inputCalibrationData.defaultCenterEyeMat) * inputCalibrationData.defaultHeadMat;
    controller::Pose hmdHeadPose = pose.transform(sensorToAvatar);
    _poseStateMap[controller::HEAD] = hmdHeadPose.postTransform(defaultHeadOffset);
}

void ViveControllerManager::InputDevice::handlePoseEvent(float deltaTime, const controller::InputCalibrationData& inputCalibrationData,
                                                         const mat4& mat, const vec3& linearVelocity,
                                                         const vec3& angularVelocity, bool isLeftHand) {
    auto pose = openVrControllerPoseToHandPose(isLeftHand, mat, linearVelocity, angularVelocity);

    // transform into avatar frame
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    _poseStateMap[isLeftHand ? controller::LEFT_HAND : controller::RIGHT_HAND] = pose.transform(controllerToAvatar);
}

bool ViveControllerManager::InputDevice::triggerHapticPulse(float strength, float duration, controller::Hand hand) {
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

void ViveControllerManager::InputDevice::calibrateFeet(glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    auto& firstFoot = _validTrackedObjects[FIRST_FOOT];
    auto& secondFoot = _validTrackedObjects[SECOND_FOOT];
    controller::Pose& firstFootPose = firstFoot.second;
    controller::Pose& secondFootPose = secondFoot.second;
    
    if (firstFootPose.translation.x < secondFootPose.translation.x) {
        _jointToPuckMap[controller::LEFT_FOOT] = firstFoot.first;
        _pucksOffset[firstFoot.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultLeftFoot, firstFootPose);
        _jointToPuckMap[controller::RIGHT_FOOT] = secondFoot.first;
        _pucksOffset[secondFoot.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultRightFoot, secondFootPose);   
    } else {
        _jointToPuckMap[controller::LEFT_FOOT] = secondFoot.first;
        _pucksOffset[secondFoot.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultLeftFoot, secondFootPose);
        _jointToPuckMap[controller::RIGHT_FOOT] = firstFoot.first;
        _pucksOffset[firstFoot.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultRightFoot, firstFootPose);
    }
}


void ViveControllerManager::InputDevice::calibrateFeet(glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, glm::vec3 headXAxis, glm::vec3 headPosition) {
    auto& firstFoot = _validTrackedObjects[FIRST_FOOT];
    auto& secondFoot = _validTrackedObjects[SECOND_FOOT];
    controller::Pose& firstFootPose = firstFoot.second;
    controller::Pose& secondFootPose = secondFoot.second;
    
    if (determineFeetOrdering(firstFootPose, secondFootPose, headXAxis, headPosition)) {
        _jointToPuckMap[controller::LEFT_FOOT] = firstFoot.first;
        _pucksOffset[firstFoot.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultLeftFoot, firstFootPose);
        _jointToPuckMap[controller::RIGHT_FOOT] = secondFoot.first;
        _pucksOffset[secondFoot.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultRightFoot, secondFootPose);   
    } else {
        _jointToPuckMap[controller::LEFT_FOOT] = secondFoot.first;
        _pucksOffset[secondFoot.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultLeftFoot, secondFootPose);
        _jointToPuckMap[controller::RIGHT_FOOT] = firstFoot.first;
        _pucksOffset[firstFoot.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultRightFoot, firstFootPose);
    }
}

void ViveControllerManager::InputDevice::calibrateHips(glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    _jointToPuckMap[controller::HIPS] = _validTrackedObjects[HIP].first;
    _pucksOffset[_validTrackedObjects[HIP].first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultHips, _validTrackedObjects[HIP].second);
}

void ViveControllerManager::InputDevice::calibrateChest(glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    _jointToPuckMap[controller::SPINE2] = _validTrackedObjects[CHEST].first;
    _pucksOffset[_validTrackedObjects[CHEST].first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultSpine2, _validTrackedObjects[CHEST].second);
}

void ViveControllerManager::InputDevice::calibrateShoulders(glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration,
                                                            int firstShoulderIndex, int secondShoulderIndex) {
    const PuckPosePair& firstShoulder = _validTrackedObjects[firstShoulderIndex];
    const PuckPosePair& secondShoulder = _validTrackedObjects[secondShoulderIndex];
    const controller::Pose& firstShoulderPose = firstShoulder.second;
    const controller::Pose& secondShoulderPose = secondShoulder.second;

    if (firstShoulderPose.translation.x < secondShoulderPose.translation.x) {
        _jointToPuckMap[controller::LEFT_ARM] = firstShoulder.first;
        _pucksOffset[firstShoulder.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultLeftArm, firstShoulder.second);
        _jointToPuckMap[controller::RIGHT_ARM] = secondShoulder.first;
        _pucksOffset[secondShoulder.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultRightArm, secondShoulder.second);
    } else {
        _jointToPuckMap[controller::LEFT_ARM] = secondShoulder.first;
        _pucksOffset[secondShoulder.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultLeftArm, secondShoulder.second);
        _jointToPuckMap[controller::RIGHT_ARM] = firstShoulder.first;
        _pucksOffset[firstShoulder.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultRightArm, firstShoulder.second);
    }
}

void ViveControllerManager::InputDevice::calibrateHead(glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration) {
    size_t headIndex = _validTrackedObjects.size() - 1;
    const PuckPosePair& head = _validTrackedObjects[headIndex];

    // assume the person is wearing the head puck on his/her forehead
    glm::mat4 defaultHeadOffset = glm::inverse(inputCalibration.defaultCenterEyeMat) * inputCalibration.defaultHeadMat;
    controller::Pose newHead = head.second.postTransform(defaultHeadOffset);

    _jointToPuckMap[controller::HEAD] = head.first;
    _pucksOffset[head.first] = computeOffset(defaultToReferenceMat, inputCalibration.defaultHeadMat, newHead);
}


void ViveControllerManager::InputDevice::loadSettings() {
    Settings settings;
    settings.beginGroup("PUCK_CONFIG");
    {
        _preferedConfig = (Config)settings.value("configuration", QVariant((int)Config::Auto)).toInt();
    }
    settings.endGroup();
}

void ViveControllerManager::InputDevice::saveSettings() const {
    Settings settings;
    settings.beginGroup("PUCK_CONFIG");
    {
        settings.setValue(QString("configuration"), (int)_preferedConfig);
    }
    settings.endGroup();
}

QString ViveControllerManager::InputDevice::configToString(Config config) {
    return _configStringMap[config];
}

void ViveControllerManager::InputDevice::setConfigFromString(const QString& value) {
    if (value ==  "Auto") {
        _preferedConfig = Config::Auto;
    } else if (value == "Feet") {
        _preferedConfig = Config::Feet;
    } else if (value == "FeetAndHips") {
        _preferedConfig = Config::FeetAndHips;
    } else if (value == "FeetHipsAndChest") {
        _preferedConfig = Config::FeetHipsAndChest;
    } else if (value == "FeetHipsAndShoulders") {
        _preferedConfig = Config::FeetHipsAndShoulders;
    } else if (value == "FeetHipsChestAndHead") {
        _preferedConfig = Config::FeetHipsChestAndHead;
    } else if (value == "FeetHipsAndHead") {
        _preferedConfig = Config::FeetHipsAndHead;
    }
}

void ViveControllerManager::InputDevice::createPreferences() {
    loadSettings();
    auto preferences = DependencyManager::get<Preferences>();
    static const QString VIVE_PUCKS_CONFIG = "Vive Pucks Configuration";

    {
        static const float MIN_VALUE = -3.0f;
        static const float MAX_VALUE = 3.0f;
        static const float STEP = 0.01f;

        auto getter = [this]()->float { return HEAD_PUCK_Y_OFFSET; };
        auto setter = [this](const float& value) { HEAD_PUCK_Y_OFFSET = value; };

        auto preference = new SpinnerPreference(VIVE_PUCKS_CONFIG, "HeadPuckYOffset", getter, setter);
        preference->setMin(MIN_VALUE);
        preference->setMax(MAX_VALUE);
        preference->setDecimals(3);
        preference->setStep(STEP);
        preferences->addPreference(preference);
    }

    {
        static const float MIN_VALUE = -3.0f;
        static const float MAX_VALUE = 3.0f;
        static const float STEP = 0.01f;

        auto getter = [this]()->float { return HEAD_PUCK_Z_OFFSET; };
        auto setter = [this](const float& value) { HEAD_PUCK_Z_OFFSET = value; };

        auto preference = new SpinnerPreference(VIVE_PUCKS_CONFIG, "HeadPuckXOffset", getter, setter);
        preference->setMin(MIN_VALUE);
        preference->setMax(MAX_VALUE);
        preference->setStep(STEP);
        preference->setDecimals(3);
        preferences->addPreference(preference);
    }
        
    {
        auto getter = [this]()->QString { return _configStringMap[_preferedConfig]; };
        auto setter = [this](const QString& value) { setConfigFromString(value); saveSettings(); };
        auto preference = new ComboBoxPreference(VIVE_PUCKS_CONFIG, "Configuration", getter, setter);
        QStringList list = {"Auto", "Feet", "FeetAndHips", "FeetHipsAndChest", "FeetHipsAndShoulders", "FeetHipsAndHead"};
        preference->setItems(list);
        preferences->addPreference(preference);

    }
}

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

        // 3d location of controller
        makePair(LEFT_HAND, "LeftHand"),
        makePair(RIGHT_HAND, "RightHand"),
        makePair(LEFT_FOOT, "LeftFoot"),
        makePair(RIGHT_FOOT, "RightFoot"),
        makePair(HIPS, "Hips"),
        makePair(SPINE2, "Spine2"),
        makePair(HEAD, "Head"),
        makePair(LEFT_ARM, "LeftArm"),
        makePair(RIGHT_ARM, "RightArm"),

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
