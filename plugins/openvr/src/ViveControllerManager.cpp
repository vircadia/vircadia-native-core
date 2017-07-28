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
#include <ui-plugins/PluginContainer.h>
#include <plugins/DisplayPlugin.h>

#include <controllers/UserInputMapper.h>
#include <Plugins/InputConfiguration.h>
#include <controllers/StandardControls.h>

extern PoseData _nextSimPoseData;

vr::IVRSystem* acquireOpenVrSystem();
void releaseOpenVrSystem();

static const QString OPENVR_LAYOUT = QString("OpenVrConfiguration.qml");
static const char* CONTROLLER_MODEL_STRING = "vr_controller_05_wireless_b";
const quint64 CALIBRATION_TIMELAPSE = 1 * USECS_PER_SECOND;

static const char* MENU_PARENT = "Avatar";
static const char* MENU_NAME = "Vive Controllers";
static const char* MENU_PATH = "Avatar" ">" "Vive Controllers";
static const char* RENDER_CONTROLLERS = "Render Hand Controllers";

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

static glm::mat4 calculateResetMat() {
    auto chaperone = vr::VRChaperone();
    if (chaperone) {
        float const UI_RADIUS = 1.0f;
        float const UI_HEIGHT = 1.6f;
        float const UI_Z_OFFSET = 0.5;

        float xSize, zSize;
        chaperone->GetPlayAreaSize(&xSize, &zSize);
        glm::vec3 uiPos(0.0f, UI_HEIGHT, UI_RADIUS - (0.5f * zSize) - UI_Z_OFFSET);

        return glm::inverse(createMatFromQuatAndPos(glm::quat(), uiPos));
    }
    return glm::mat4();
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
            if (!_desktopMode) {
                _resetMatCalculated = false;
            }
        }
        _inputDevice->configureCalibrationSettings(configurationSettings);
        saveSettings();
    }
}

QJsonObject ViveControllerManager::configurationSettings() {
    if (isSupported()) {
        QJsonObject configurationSettings = _inputDevice->configurationSettings();
        configurationSettings["desktopMode"] = _desktopMode;
        return configurationSettings;
    }

    return QJsonObject();
}

QString ViveControllerManager::configurationLayout() {
    return OPENVR_LAYOUT;
}

bool ViveControllerManager::activate() {
    InputPlugin::activate();

    loadSettings();

    if (!_system) {
        _system = acquireOpenVrSystem();
    }

    if (!_system) {
        return false;
    }

    _container->addMenu(MENU_PATH);
    _container->addMenuItem(PluginType::INPUT_PLUGIN, MENU_PATH, RENDER_CONTROLLERS,
        [this](bool clicked) { this->setRenderControllers(clicked); },
        true, true);

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

    saveSettings();
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

    if (isDesktopMode() && _desktopMode) {
        if (!_resetMatCalculated) {
            _resetMat = calculateResetMat();
            _resetMatCalculated = true;
        }

        _system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, _nextSimPoseData.vrPoses, vr::k_unMaxTrackedDeviceCount);
        _nextSimPoseData.update(_resetMat);
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
}

void ViveControllerManager::loadSettings() {
    Settings settings;
    QString nameString = getName();
    settings.beginGroup(nameString);
    {
        if (_inputDevice) {
            const double DEFAULT_ARM_CIRCUMFERENCE = 0.33;
            const double DEFAULT_SHOULDER_WIDTH = 0.48;
            _inputDevice->_armCircumference = settings.value("armCircumference", QVariant(DEFAULT_ARM_CIRCUMFERENCE)).toDouble();
            _inputDevice->_shoulderWidth = settings.value("shoulderWidth", QVariant(DEFAULT_SHOULDER_WIDTH)).toDouble();
        }
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
        }
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

    calibrateFromHandController(inputCalibrationData);
    calibrateFromUI(inputCalibrationData);

    updateCalibratedLimbs();
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
        while (iter != end) {
            if (iter.key() == "bodyConfiguration") {
                setConfigFromString(iter.value().toString());
            } else if (iter.key() == "headConfiguration") {
                QJsonObject headObject = iter.value().toObject();
                bool overrideHead = headObject["override"].toBool();
                if (overrideHead) {
                    _headConfig = HeadConfig::Puck;
                    _headPuckYOffset = headObject["Y"].toDouble() * CM_TO_M;
                    _headPuckZOffset = headObject["Z"].toDouble() * CM_TO_M;
                } else {
                    _headConfig = HeadConfig::HMD;
                }
            } else if (iter.key() == "handConfiguration") {
                QJsonObject handsObject = iter.value().toObject();
                bool overrideHands = handsObject["override"].toBool();
                if (overrideHands) {
                    _handConfig = HandConfig::Pucks;
                    _handPuckYOffset = handsObject["Y"].toDouble() * CM_TO_M;
                    _handPuckZOffset = handsObject["Z"].toDouble() * CM_TO_M;
                } else {
                    _handConfig = HandConfig::HandController;
                }
            } else if (iter.key() == "armCircumference") {
                _armCircumference = (float)iter.value().toDouble() * CM_TO_M;
            } else if (iter.key() == "shoulderWidth") {
                _shoulderWidth = (float)iter.value().toDouble() * CM_TO_M;
            }
            iter++;
        }
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
    configurationSettings["armCircumference"] = (double)_armCircumference * M_TO_CM;
    configurationSettings["shoulderWidth"] = (double)_shoulderWidth * M_TO_CM;
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

        // but _validTrackedObjects remain in sensor frame
        _validTrackedObjects.push_back(std::make_pair(poseIndex, pose));
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
    std::sort(_validTrackedObjects.begin(), _validTrackedObjects.end(), sortPucksXPosition);
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
    glm::vec3 headXAxis = getReferenceHeadXAxis(defaultToReferenceMat, inputCalibration.defaultHeadMat);
    glm::vec3 headPosition = getReferenceHeadPosition(defaultToReferenceMat, inputCalibration.defaultHeadMat);
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

    if (_overrideHands) {
        _poseStateMap[controller::LEFT_HAND] = addOffsetToPuckPose(controller::LEFT_HAND);
        _poseStateMap[controller::RIGHT_HAND] = addOffsetToPuckPose(controller::RIGHT_HAND);
    }
}

controller::Pose ViveControllerManager::InputDevice::addOffsetToPuckPose(int joint) const {
    auto puck = _jointToPuckMap.find(joint);
    if (puck != _jointToPuckMap.end()) {
        uint32_t puckIndex = puck->second;
        auto puckPose = _poseStateMap.find(puckIndex);
        auto puckPostOffset = _pucksPostOffset.find(puckIndex);
        auto puckPreOffset = _pucksPreOffset.find(puckIndex);

        if (puckPose != _poseStateMap.end()) {
            if (puckPreOffset != _pucksPreOffset.end() && puckPostOffset != _pucksPostOffset.end()) {
                return puckPose->second.postTransform(puckPostOffset->second).transform(puckPreOffset->second);
            } else if (puckPostOffset != _pucksPostOffset.end()) {
                return puckPose->second.postTransform(puckPostOffset->second);
            } else if (puckPreOffset != _pucksPreOffset.end()) {
                return puckPose->second.transform(puckPreOffset->second);
            }
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
    glm::vec3 centerEyeTrans = headPuckPose.translation + centerEyeRotMat * glm::vec3(0.0f, _headPuckYOffset, _headPuckZOffset);

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

void ViveControllerManager::InputDevice::calibrateLeftHand(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, PuckPosePair& handPair) {
    controller::Pose& handPose = handPair.second;
    glm::mat4 handPoseAvatarMat = createMatFromQuatAndPos(handPose.getRotation(), handPose.getTranslation());
    glm::vec3 handPoseTranslation = extractTranslation(handPoseAvatarMat);
    glm::vec3 handPoseZAxis = glmExtractRotation(handPoseAvatarMat) * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 avatarHandYAxis = transformVectorFast(inputCalibration.defaultLeftHand, glm::vec3(0.0f, 1.0f, 0.0f));
    const float EPSILON = 1.0e-4f;
    if (fabsf(fabsf(glm::dot(glm::normalize(avatarHandYAxis), glm::normalize(handPoseZAxis))) - 1.0f) < EPSILON) {
        handPoseZAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::vec3 zPrime = handPoseZAxis;
    glm::vec3 xPrime = glm::normalize(glm::cross(avatarHandYAxis, handPoseZAxis));
    glm::vec3 yPrime = glm::normalize(glm::cross(zPrime, xPrime));

    glm::mat4 newHandMat = glm::mat4(glm::vec4(xPrime, 0.0f), glm::vec4(yPrime, 0.0f),
                                     glm::vec4(zPrime, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));


    glm::vec3 translationOffset = glm::vec3(0.0f, _handPuckYOffset, _handPuckZOffset);
    glm::quat initialRotation = glmExtractRotation(handPoseAvatarMat);
    glm::quat finalRotation = glmExtractRotation(newHandMat);

    glm::quat rotationOffset = glm::inverse(initialRotation) * finalRotation;

    glm::mat4 offsetMat = createMatFromQuatAndPos(rotationOffset, translationOffset);

    _jointToPuckMap[controller::LEFT_HAND] = handPair.first;
    _pucksPostOffset[handPair.first] = offsetMat;
}

void ViveControllerManager::InputDevice::calibrateRightHand(const glm::mat4& defaultToReferenceMat, const controller::InputCalibrationData& inputCalibration, PuckPosePair& handPair) {
    controller::Pose& handPose = handPair.second;
    glm::mat4 handPoseAvatarMat = createMatFromQuatAndPos(handPose.getRotation(), handPose.getTranslation());
    glm::vec3 handPoseTranslation = extractTranslation(handPoseAvatarMat);
    glm::vec3 handPoseZAxis = glmExtractRotation(handPoseAvatarMat) * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 avatarHandYAxis = transformVectorFast(inputCalibration.defaultRightHand, glm::vec3(0.0f, 1.0f, 0.0f));
    const float EPSILON = 1.0e-4f;
    if (fabsf(fabsf(glm::dot(glm::normalize(avatarHandYAxis), glm::normalize(handPoseZAxis))) - 1.0f) < EPSILON) {
        handPoseZAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::vec3 zPrime = handPoseZAxis;
    glm::vec3 xPrime = glm::normalize(glm::cross(avatarHandYAxis, handPoseZAxis));
    glm::vec3 yPrime = glm::normalize(glm::cross(zPrime, xPrime));
    glm::mat4 newHandMat = glm::mat4(glm::vec4(xPrime, 0.0f), glm::vec4(yPrime, 0.0f),
                                     glm::vec4(zPrime, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));



    glm::vec3 translationOffset = glm::vec3(0.0f, _handPuckYOffset, _handPuckZOffset);
    glm::quat initialRotation = glmExtractRotation(handPoseAvatarMat);
    glm::quat finalRotation = glmExtractRotation(newHandMat);

    glm::quat rotationOffset = glm::inverse(initialRotation) * finalRotation;

    glm::mat4 offsetMat = createMatFromQuatAndPos(rotationOffset, translationOffset);

    _jointToPuckMap[controller::RIGHT_HAND] = handPair.first;
    _pucksPostOffset[handPair.first] = offsetMat;
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
    glm::mat4 puckPoseAvatarMat = createMatFromQuatAndPos(footPose.getRotation(), footPose.getTranslation());
    glm::mat4 defaultFoot = isLeftFoot ? inputCalibration.defaultLeftFoot : inputCalibration.defaultRightFoot;
    glm::mat4 footOffset = computeOffset(defaultToReferenceMat, defaultFoot, footPose);

    glm::quat rotationOffset = glmExtractRotation(footOffset);
    glm::vec3 translationOffset = extractTranslation(footOffset);
    glm::vec3 avatarXAxisInPuckFrame = glm::normalize(transformVectorFast(glm::inverse(puckPoseAvatarMat), glm::vec3(-1.0f, 0.0f, 0.0f)));
    float distance = glm::dot(translationOffset, avatarXAxisInPuckFrame);
    glm::vec3 finalTranslation =  translationOffset - (distance * avatarXAxisInPuckFrame);
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
