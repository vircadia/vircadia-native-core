//
//  LeapMotionPlugin.cpp
//
//  Created by David Rowe on 15 Jun 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LeapMotionPlugin.h"

#include <QLoggingCategory>

#include <controllers/UserInputMapper.h>
#include <NumericalConstants.h>
#include <PathUtils.h>
#include <Preferences.h>
#include <SettingHandle.h>

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

const char* LeapMotionPlugin::NAME = "LeapMotion";
const char* LeapMotionPlugin::LEAPMOTION_ID_STRING = "Leap Motion";

const bool DEFAULT_ENABLED = false;
const char* SENSOR_ON_DESKTOP = "Desktop";
const char* SENSOR_ON_HMD = "HMD";
const char* DEFAULT_SENSOR_LOCATION = SENSOR_ON_DESKTOP;

enum LeapMotionJointIndex {
    LeftHand = 0,
    LeftHandThumb1,
    LeftHandThumb2,
    LeftHandThumb3,
    LeftHandThumb4,
    LeftHandIndex1,
    LeftHandIndex2,
    LeftHandIndex3,
    LeftHandIndex4,
    LeftHandMiddle1,
    LeftHandMiddle2,
    LeftHandMiddle3,
    LeftHandMiddle4,
    LeftHandRing1,
    LeftHandRing2,
    LeftHandRing3,
    LeftHandRing4,
    LeftHandPinky1,
    LeftHandPinky2,
    LeftHandPinky3,
    LeftHandPinky4,
    RightHand,
    RightHandThumb1,
    RightHandThumb2,
    RightHandThumb3,
    RightHandThumb4,
    RightHandIndex1,
    RightHandIndex2,
    RightHandIndex3,
    RightHandIndex4,
    RightHandMiddle1,
    RightHandMiddle2,
    RightHandMiddle3,
    RightHandMiddle4,
    RightHandRing1,
    RightHandRing2,
    RightHandRing3,
    RightHandRing4,
    RightHandPinky1,
    RightHandPinky2,
    RightHandPinky3,
    RightHandPinky4,

    Size
};

static controller::StandardPoseChannel LeapMotionJointIndexToPoseIndexMap[LeapMotionJointIndex::Size] = {
    controller::LEFT_HAND,
    controller::LEFT_HAND_THUMB1,
    controller::LEFT_HAND_THUMB2,
    controller::LEFT_HAND_THUMB3,
    controller::LEFT_HAND_THUMB4,
    controller::LEFT_HAND_INDEX1,
    controller::LEFT_HAND_INDEX2,
    controller::LEFT_HAND_INDEX3,
    controller::LEFT_HAND_INDEX4,
    controller::LEFT_HAND_MIDDLE1,
    controller::LEFT_HAND_MIDDLE2,
    controller::LEFT_HAND_MIDDLE3,
    controller::LEFT_HAND_MIDDLE4,
    controller::LEFT_HAND_RING1,
    controller::LEFT_HAND_RING2,
    controller::LEFT_HAND_RING3,
    controller::LEFT_HAND_RING4,
    controller::LEFT_HAND_PINKY1,
    controller::LEFT_HAND_PINKY2,
    controller::LEFT_HAND_PINKY3,
    controller::LEFT_HAND_PINKY4,
    controller::RIGHT_HAND,
    controller::RIGHT_HAND_THUMB1,
    controller::RIGHT_HAND_THUMB2,
    controller::RIGHT_HAND_THUMB3,
    controller::RIGHT_HAND_THUMB4,
    controller::RIGHT_HAND_INDEX1,
    controller::RIGHT_HAND_INDEX2,
    controller::RIGHT_HAND_INDEX3,
    controller::RIGHT_HAND_INDEX4,
    controller::RIGHT_HAND_MIDDLE1,
    controller::RIGHT_HAND_MIDDLE2,
    controller::RIGHT_HAND_MIDDLE3,
    controller::RIGHT_HAND_MIDDLE4,
    controller::RIGHT_HAND_RING1,
    controller::RIGHT_HAND_RING2,
    controller::RIGHT_HAND_RING3,
    controller::RIGHT_HAND_RING4,
    controller::RIGHT_HAND_PINKY1,
    controller::RIGHT_HAND_PINKY2,
    controller::RIGHT_HAND_PINKY3,
    controller::RIGHT_HAND_PINKY4
};

#define UNKNOWN_JOINT (controller::StandardPoseChannel)0 

static controller::StandardPoseChannel LeapMotionJointIndexToPoseIndex(LeapMotionJointIndex i) {
    assert(i >= 0 && i < LeapMotionJointIndex::Size);
    if (i >= 0 && i < LeapMotionJointIndex::Size) {
        return LeapMotionJointIndexToPoseIndexMap[i];
    } else {
        return UNKNOWN_JOINT;
    }
}

QStringList controllerJointNames = {
    "Hips",
    "RightUpLeg",
    "RightLeg",
    "RightFoot",
    "LeftUpLeg",
    "LeftLeg",
    "LeftFoot",
    "Spine",
    "Spine1",
    "Spine2",
    "Spine3",
    "Neck",
    "Head",
    "RightShoulder",
    "RightArm",
    "RightForeArm",
    "RightHand",
    "RightHandThumb1",
    "RightHandThumb2",
    "RightHandThumb3",
    "RightHandThumb4",
    "RightHandIndex1",
    "RightHandIndex2",
    "RightHandIndex3",
    "RightHandIndex4",
    "RightHandMiddle1",
    "RightHandMiddle2",
    "RightHandMiddle3",
    "RightHandMiddle4",
    "RightHandRing1",
    "RightHandRing2",
    "RightHandRing3",
    "RightHandRing4",
    "RightHandPinky1",
    "RightHandPinky2",
    "RightHandPinky3",
    "RightHandPinky4",
    "LeftShoulder",
    "LeftArm",
    "LeftForeArm",
    "LeftHand",
    "LeftHandThumb1",
    "LeftHandThumb2",
    "LeftHandThumb3",
    "LeftHandThumb4",
    "LeftHandIndex1",
    "LeftHandIndex2",
    "LeftHandIndex3",
    "LeftHandIndex4",
    "LeftHandMiddle1",
    "LeftHandMiddle2",
    "LeftHandMiddle3",
    "LeftHandMiddle4",
    "LeftHandRing1",
    "LeftHandRing2",
    "LeftHandRing3",
    "LeftHandRing4",
    "LeftHandPinky1",
    "LeftHandPinky2",
    "LeftHandPinky3",
    "LeftHandPinky4"
};

static const char* getControllerJointName(controller::StandardPoseChannel i) {
    if (i >= 0 && i < controller::NUM_STANDARD_POSES) {
        return qPrintable(controllerJointNames[i]);
    }
    return "unknown";
}

void LeapMotionPlugin::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    if (!_enabled) {
        return;
    }

    const auto frame = _controller.frame();
    const auto frameID = frame.id();
    if (_lastFrameID >= frameID) {
        // Leap Motion not connected or duplicate frame.
        return;
    }

    if (!_hasLeapMotionBeenConnected) {
        emit deviceConnected(getName());
        _hasLeapMotionBeenConnected = true;
    }

    processFrame(frame);  // Updates _joints.

    auto joints = _joints;

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData, joints, _prevJoints);
    });

    _prevJoints = joints;

    _lastFrameID = frameID;
}

controller::Input::NamedVector LeapMotionPlugin::InputDevice::getAvailableInputs() const {
    static controller::Input::NamedVector availableInputs;
    if (availableInputs.size() == 0) {
        for (int i = 0; i < LeapMotionJointIndex::Size; i++) {
            auto channel = LeapMotionJointIndexToPoseIndex(static_cast<LeapMotionJointIndex>(i));
            availableInputs.push_back(makePair(channel, getControllerJointName(channel)));
        }
    };
    return availableInputs;
}

QString LeapMotionPlugin::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/leapmotion.json";
    return MAPPING_JSON;
}

void LeapMotionPlugin::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData,
        const std::vector<LeapMotionPlugin::LeapMotionJoint>& joints,
        const std::vector<LeapMotionPlugin::LeapMotionJoint>& prevJoints) {

    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    glm::quat controllerToAvatarRotation = glmExtractRotation(controllerToAvatar);

    glm::vec3 hmdSensorPosition;    // HMD
    glm::quat hmdSensorOrientation; // HMD
    glm::vec3 leapMotionOffset;     // Desktop
    if (_isLeapOnHMD) {
        hmdSensorPosition = extractTranslation(inputCalibrationData.hmdSensorMat);
        hmdSensorOrientation = extractRotation(inputCalibrationData.hmdSensorMat);
    } else {
        // Desktop "zero" position is some distance above the Leap Motion sensor and half the avatar's shoulder-to-hand length 
        // in front of avatar.
        float halfShouldToHandLength = fabsf(extractTranslation(inputCalibrationData.defaultLeftHand).x
            - extractTranslation(inputCalibrationData.defaultLeftArm).x) / 2.0f;
        leapMotionOffset = glm::vec3(0.0f, _desktopHeightOffset, halfShouldToHandLength);
    }

    for (size_t i = 0; i < joints.size(); i++) {
        int poseIndex = LeapMotionJointIndexToPoseIndex((LeapMotionJointIndex)i);

        if (joints[i].position == Vectors::ZERO) {
            _poseStateMap[poseIndex] = controller::Pose();
            continue;
        }

        glm::vec3 pos;
        glm::quat rot;
        glm::quat prevRot;
        if (_isLeapOnHMD) {
            auto jointPosition = joints[i].position;
            const glm::vec3 HMD_EYE_TO_LEAP_OFFSET = glm::vec3(0.0f, 0.0f, -0.09f);  // Eyes to surface of Leap Motion.
            jointPosition = glm::vec3(-jointPosition.x, -jointPosition.z, -jointPosition.y) + HMD_EYE_TO_LEAP_OFFSET;
            jointPosition = hmdSensorPosition + hmdSensorOrientation * jointPosition;
            pos = transformPoint(controllerToAvatar, jointPosition);

            glm::quat jointOrientation = joints[i].orientation;
            jointOrientation = glm::quat(jointOrientation.w, -jointOrientation.x, -jointOrientation.z, -jointOrientation.y);
            rot = controllerToAvatarRotation * hmdSensorOrientation * jointOrientation;

            glm::quat prevJointOrientation = prevJoints[i].orientation;
            prevJointOrientation =
                glm::quat(prevJointOrientation.w, -prevJointOrientation.x, -prevJointOrientation.z, -prevJointOrientation.y);
            prevRot = controllerToAvatarRotation * hmdSensorOrientation * prevJointOrientation;

        } else {
            pos = controllerToAvatarRotation * (joints[i].position - leapMotionOffset);
            const glm::quat ZERO_HAND_ORIENTATION = glm::quat(glm::vec3(PI_OVER_TWO, PI, 0.0f));
            rot = controllerToAvatarRotation * joints[i].orientation * ZERO_HAND_ORIENTATION;
            prevRot = controllerToAvatarRotation * prevJoints[i].orientation * ZERO_HAND_ORIENTATION;
        }

        glm::vec3 linearVelocity, angularVelocity;
        if (i < prevJoints.size()) {
            linearVelocity = (pos - (prevJoints[i].position * METERS_PER_CENTIMETER)) / deltaTime;  // m/s
            // quat log imaginary part points along the axis of rotation, with length of one half the angle of rotation.
            glm::quat d = glm::log(rot * glm::inverse(prevRot));
            angularVelocity = glm::vec3(d.x, d.y, d.z) / (0.5f * deltaTime); // radians/s
        }

        _poseStateMap[poseIndex] = controller::Pose(pos, rot, linearVelocity, angularVelocity);
    }
}

void LeapMotionPlugin::init() {
    loadSettings();
    auto preferences = DependencyManager::get<Preferences>();
    static const QString LEAPMOTION_PLUGIN { "Leap Motion" };
    {
        auto getter = [this]()->bool { return _enabled; };
        auto setter = [this](bool value) {
            _enabled = value;
            emit deviceStatusChanged(getName(), isRunning());
            saveSettings();
            if (!_enabled) {
                auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
                userInputMapper->withLock([&, this]() {
                    _inputDevice->clearState();
                });
            }
        };
        auto preference = new CheckPreference(LEAPMOTION_PLUGIN, "Enabled", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = [this]()->QString { return _sensorLocation; };
        auto setter = [this](QString value) {
            _sensorLocation = value;
            saveSettings();
            applySensorLocation();
        };
        auto preference = new ComboBoxPreference(LEAPMOTION_PLUGIN, "Sensor location", getter, setter);
        QStringList list = { SENSOR_ON_DESKTOP, SENSOR_ON_HMD };
        preference->setItems(list);
        preferences->addPreference(preference);
    }
    {
        auto getter = [this]()->float { return _desktopHeightOffset; };
        auto setter = [this](float value) {
            _desktopHeightOffset = value;
            saveSettings();
            applyDesktopHeightOffset();
        };
        auto preference = new SpinnerPreference(LEAPMOTION_PLUGIN, "Desktop height for horizontal forearms", getter, setter);
        float MIN_VALUE = 0.0f;
        float MAX_VALUE = 1.0f;
        float DECIMALS = 2.0f;
        float STEP = 0.01f;
        preference->setMin(MIN_VALUE);
        preference->setMax(MAX_VALUE);
        preference->setDecimals(DECIMALS);
        preference->setStep(STEP);
        preferences->addPreference(preference);
    }
}

bool LeapMotionPlugin::activate() {
    InputPlugin::activate();

    // Nothing required to be done to start up Leap Motion library.

    _joints.resize(LeapMotionJointIndex::Size, { glm::vec3(), glm::quat() });

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(_inputDevice);

    return true;
}

void LeapMotionPlugin::deactivate() {
    if (_inputDevice->_deviceID != controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->removeDevice(_inputDevice->_deviceID);
    }

    InputPlugin::deactivate();
}

const char* SETTINGS_ENABLED_KEY = "enabled";
const char* SETTINGS_SENSOR_LOCATION_KEY = "sensorLocation";
const char* SETTINGS_DESKTOP_HEIGHT_OFFSET_KEY = "desktopHeightOffset";

void LeapMotionPlugin::saveSettings() const {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.setValue(QString(SETTINGS_ENABLED_KEY), _enabled);
        settings.setValue(QString(SETTINGS_SENSOR_LOCATION_KEY), _sensorLocation);
        settings.setValue(QString(SETTINGS_DESKTOP_HEIGHT_OFFSET_KEY), _desktopHeightOffset);
    }
    settings.endGroup();
}

void LeapMotionPlugin::loadSettings() {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        _enabled = settings.value(SETTINGS_ENABLED_KEY, QVariant(DEFAULT_ENABLED)).toBool();
        emit deviceStatusChanged(getName(), isRunning());
        _sensorLocation = settings.value(SETTINGS_SENSOR_LOCATION_KEY, QVariant(DEFAULT_SENSOR_LOCATION)).toString();
        _desktopHeightOffset = 
            settings.value(SETTINGS_DESKTOP_HEIGHT_OFFSET_KEY, QVariant(DEFAULT_DESKTOP_HEIGHT_OFFSET)).toFloat();
        applySensorLocation();
        applyDesktopHeightOffset();
    }
    settings.endGroup();
}

void LeapMotionPlugin::InputDevice::clearState() {
    for (size_t i = 0; i < LeapMotionJointIndex::Size; i++) {
        int poseIndex = LeapMotionJointIndexToPoseIndex((LeapMotionJointIndex)i);
        _poseStateMap[poseIndex] = controller::Pose();
    }
}

void LeapMotionPlugin::applySensorLocation() {
    bool isLeapOnHMD = _sensorLocation == SENSOR_ON_HMD;
    _controller.setPolicyFlags(isLeapOnHMD ? Leap::Controller::POLICY_OPTIMIZE_HMD : Leap::Controller::POLICY_DEFAULT);
    _inputDevice->setIsLeapOnHMD(isLeapOnHMD);
}

void LeapMotionPlugin::applyDesktopHeightOffset() {
    _inputDevice->setDektopHeightOffset(_desktopHeightOffset);
}

const float LEFT_SIDE_SIGN = -1.0f;
const float RIGHT_SIDE_SIGN = 1.0f;

glm::quat LeapBasisToQuat(float sideSign, const Leap::Matrix& basis) {
    glm::vec3 xAxis = sideSign * glm::vec3(basis.xBasis.x, basis.xBasis.y, basis.xBasis.z);
    glm::vec3 yAxis = glm::vec3(basis.yBasis.x, basis.yBasis.y, basis.yBasis.z);
    glm::vec3 zAxis = glm::vec3(basis.zBasis.x, basis.zBasis.y, basis.zBasis.z);
    glm::quat orientation = (glm::quat_cast(glm::mat3(xAxis, yAxis, zAxis)));
    return orientation;
}

glm::vec3 LeapVectorToVec3(const Leap::Vector& vec) {
    return glm::vec3(vec.x * METERS_PER_MILLIMETER, vec.y * METERS_PER_MILLIMETER, vec.z * METERS_PER_MILLIMETER);
}

void LeapMotionPlugin::processFrame(const Leap::Frame& frame) {
    // Default to uncontrolled.
    for (int i = 0; i < _joints.size(); i++) {
        _joints[i].position = glm::vec3();
    }

    auto hands = frame.hands();
    const int MAX_NUMBER_OF_HANDS = 2;
    for (int i = 0; i < hands.count() && i < MAX_NUMBER_OF_HANDS; i++) {
        auto hand = hands[i];

        int sideSign = hand.isLeft() ? LEFT_SIDE_SIGN : RIGHT_SIDE_SIGN;
        int jointIndex = hand.isLeft() ? LeapMotionJointIndex::LeftHand : LeapMotionJointIndex::RightHand;

        // Hand.
        _joints[jointIndex].position = LeapVectorToVec3(hand.wristPosition());
        _joints[jointIndex].orientation = LeapBasisToQuat(sideSign, hand.basis());

        // Fingers.
        // Leap Motion SDK guarantees full set of fingers and finger joints so can straightforwardly process them all.
        Leap::FingerList fingers = hand.fingers();
        for (int j = Leap::Finger::Type::TYPE_THUMB; j <= Leap::Finger::Type::TYPE_PINKY; j++) {
            Leap::Finger finger;
            finger = fingers[j];
            Leap::Bone bone;
            bone = finger.bone(Leap::Bone::Type::TYPE_PROXIMAL);
            jointIndex++;
            _joints[jointIndex].position = LeapVectorToVec3(bone.prevJoint());
            _joints[jointIndex].orientation = LeapBasisToQuat(sideSign, bone.basis());
            bone = finger.bone(Leap::Bone::Type::TYPE_INTERMEDIATE);
            jointIndex++;
            _joints[jointIndex].position = LeapVectorToVec3(bone.prevJoint());
            _joints[jointIndex].orientation = LeapBasisToQuat(sideSign, bone.basis());
            bone = finger.bone(Leap::Bone::Type::TYPE_DISTAL);
            jointIndex++;
            _joints[jointIndex].position = LeapVectorToVec3(bone.prevJoint());
            _joints[jointIndex].orientation = LeapBasisToQuat(sideSign, bone.basis());
            jointIndex++;
            _joints[jointIndex].position = LeapVectorToVec3(bone.nextJoint());
            _joints[jointIndex].orientation = LeapBasisToQuat(sideSign, bone.basis());
        }
    }
}

