//
//  NeuronPlugin.cpp
//  input-plugins/src/input-plugins
//
//  Created by Anthony Thibault on 12/18/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NeuronPlugin.h"

#include <controllers/UserInputMapper.h>
#include <QLoggingCategory>
#include <PathUtils.h>
#include <DebugDraw.h>
#include <cassert>
#include <NumericalConstants.h>
#include <StreamUtils.h>
#include <Preferences.h>
#include <SettingHandle.h>

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

#define __OS_XUN__ 1
#define BOOL int

#include <NeuronDataReader.h>

const char* NeuronPlugin::NAME = "Neuron";
const char* NeuronPlugin::NEURON_ID_STRING = "Perception Neuron";

const bool DEFAULT_ENABLED = false;
const QString DEFAULT_SERVER_ADDRESS = "localhost";
const int DEFAULT_SERVER_PORT = 7001;

// indices of joints of the Neuron standard skeleton.
// This is 'almost' the same as the High Fidelity standard skeleton.
// It is missing a thumb joint.
enum NeuronJointIndex {
    Hips = 0,
    RightUpLeg,
    RightLeg,
    RightFoot,
    LeftUpLeg,
    LeftLeg,
    LeftFoot,
    Spine,
    Spine1,
    Spine2,
    Spine3,
    Neck,
    Head,
    RightShoulder,
    RightArm,
    RightForeArm,
    RightHand,
    RightHandThumb1,
    RightHandThumb2,
    RightHandThumb3,
    RightInHandIndex,
    RightHandIndex1,
    RightHandIndex2,
    RightHandIndex3,
    RightInHandMiddle,
    RightHandMiddle1,
    RightHandMiddle2,
    RightHandMiddle3,
    RightInHandRing,
    RightHandRing1,
    RightHandRing2,
    RightHandRing3,
    RightInHandPinky,
    RightHandPinky1,
    RightHandPinky2,
    RightHandPinky3,
    LeftShoulder,
    LeftArm,
    LeftForeArm,
    LeftHand,
    LeftHandThumb1,
    LeftHandThumb2,
    LeftHandThumb3,
    LeftInHandIndex,
    LeftHandIndex1,
    LeftHandIndex2,
    LeftHandIndex3,
    LeftInHandMiddle,
    LeftHandMiddle1,
    LeftHandMiddle2,
    LeftHandMiddle3,
    LeftInHandRing,
    LeftHandRing1,
    LeftHandRing2,
    LeftHandRing3,
    LeftInHandPinky,
    LeftHandPinky1,
    LeftHandPinky2,
    LeftHandPinky3,
    Size
};

// Almost a direct mapping except for LEFT_HAND_THUMB1 and RIGHT_HAND_THUMB1,
// which are not present in the Neuron standard skeleton.
static controller::StandardPoseChannel neuronJointIndexToPoseIndexMap[NeuronJointIndex::Size] = {
    controller::HIPS,
    controller::RIGHT_UP_LEG,
    controller::RIGHT_LEG,
    controller::RIGHT_FOOT,
    controller::LEFT_UP_LEG,
    controller::LEFT_LEG,
    controller::LEFT_FOOT,
    controller::SPINE,
    controller::SPINE1,
    controller::SPINE2,
    controller::SPINE3,
    controller::NECK,
    controller::HEAD,
    controller::RIGHT_SHOULDER,
    controller::RIGHT_ARM,
    controller::RIGHT_FORE_ARM,
    controller::RIGHT_HAND,
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
    controller::RIGHT_HAND_PINKY4,
    controller::LEFT_SHOULDER,
    controller::LEFT_ARM,
    controller::LEFT_FORE_ARM,
    controller::LEFT_HAND,
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
    controller::LEFT_HAND_PINKY4
};

// in rig frame
static glm::vec3 rightHandThumb1DefaultAbsTranslation(-2.155500650405884, -0.7610001564025879, 2.685631036758423);
static glm::vec3 leftHandThumb1DefaultAbsTranslation(2.1555817127227783, -0.7603635787963867, 2.6856393814086914);

static controller::StandardPoseChannel neuronJointIndexToPoseIndex(NeuronJointIndex i) {
    assert(i >= 0 && i < NeuronJointIndex::Size);
    if (i >= 0 && i < NeuronJointIndex::Size) {
        return neuronJointIndexToPoseIndexMap[i];
    } else {
        return (controller::StandardPoseChannel)0; // not sure what to do here, but don't crash!
    }
}

static const char* controllerJointName(controller::StandardPoseChannel i) {
    switch (i) {
    case controller::HIPS: return "Hips";
    case controller::RIGHT_UP_LEG: return "RightUpLeg";
    case controller::RIGHT_LEG: return "RightLeg";
    case controller::RIGHT_FOOT: return "RightFoot";
    case controller::LEFT_UP_LEG: return "LeftUpLeg";
    case controller::LEFT_LEG: return "LeftLeg";
    case controller::LEFT_FOOT: return "LeftFoot";
    case controller::SPINE: return "Spine";
    case controller::SPINE1: return "Spine1";
    case controller::SPINE2: return "Spine2";
    case controller::SPINE3: return "Spine3";
    case controller::NECK: return "Neck";
    case controller::HEAD: return "Head";
    case controller::RIGHT_SHOULDER: return "RightShoulder";
    case controller::RIGHT_ARM: return "RightArm";
    case controller::RIGHT_FORE_ARM: return "RightForeArm";
    case controller::RIGHT_HAND: return "RightHand";
    case controller::RIGHT_HAND_THUMB1: return "RightHandThumb1";
    case controller::RIGHT_HAND_THUMB2: return "RightHandThumb2";
    case controller::RIGHT_HAND_THUMB3: return "RightHandThumb3";
    case controller::RIGHT_HAND_THUMB4: return "RightHandThumb4";
    case controller::RIGHT_HAND_INDEX1: return "RightHandIndex1";
    case controller::RIGHT_HAND_INDEX2: return "RightHandIndex2";
    case controller::RIGHT_HAND_INDEX3: return "RightHandIndex3";
    case controller::RIGHT_HAND_INDEX4: return "RightHandIndex4";
    case controller::RIGHT_HAND_MIDDLE1: return "RightHandMiddle1";
    case controller::RIGHT_HAND_MIDDLE2: return "RightHandMiddle2";
    case controller::RIGHT_HAND_MIDDLE3: return "RightHandMiddle3";
    case controller::RIGHT_HAND_MIDDLE4: return "RightHandMiddle4";
    case controller::RIGHT_HAND_RING1: return "RightHandRing1";
    case controller::RIGHT_HAND_RING2: return "RightHandRing2";
    case controller::RIGHT_HAND_RING3: return "RightHandRing3";
    case controller::RIGHT_HAND_RING4: return "RightHandRing4";
    case controller::RIGHT_HAND_PINKY1: return "RightHandPinky1";
    case controller::RIGHT_HAND_PINKY2: return "RightHandPinky2";
    case controller::RIGHT_HAND_PINKY3: return "RightHandPinky3";
    case controller::RIGHT_HAND_PINKY4: return "RightHandPinky4";
    case controller::LEFT_SHOULDER: return "LeftShoulder";
    case controller::LEFT_ARM: return "LeftArm";
    case controller::LEFT_FORE_ARM: return "LeftForeArm";
    case controller::LEFT_HAND: return "LeftHand";
    case controller::LEFT_HAND_THUMB1: return "LeftHandThumb1";
    case controller::LEFT_HAND_THUMB2: return "LeftHandThumb2";
    case controller::LEFT_HAND_THUMB3: return "LeftHandThumb3";
    case controller::LEFT_HAND_THUMB4: return "LeftHandThumb4";
    case controller::LEFT_HAND_INDEX1: return "LeftHandIndex1";
    case controller::LEFT_HAND_INDEX2: return "LeftHandIndex2";
    case controller::LEFT_HAND_INDEX3: return "LeftHandIndex3";
    case controller::LEFT_HAND_INDEX4: return "LeftHandIndex4";
    case controller::LEFT_HAND_MIDDLE1: return "LeftHandMiddle1";
    case controller::LEFT_HAND_MIDDLE2: return "LeftHandMiddle2";
    case controller::LEFT_HAND_MIDDLE3: return "LeftHandMiddle3";
    case controller::LEFT_HAND_MIDDLE4: return "LeftHandMiddle4";
    case controller::LEFT_HAND_RING1: return "LeftHandRing1";
    case controller::LEFT_HAND_RING2: return "LeftHandRing2";
    case controller::LEFT_HAND_RING3: return "LeftHandRing3";
    case controller::LEFT_HAND_RING4: return "LeftHandRing4";
    case controller::LEFT_HAND_PINKY1: return "LeftHandPinky1";
    case controller::LEFT_HAND_PINKY2: return "LeftHandPinky2";
    case controller::LEFT_HAND_PINKY3: return "LeftHandPinky3";
    case controller::LEFT_HAND_PINKY4: return "LeftHandPinky4";
    default: return "???";
    }
}

// convert between YXZ neuron euler angles in degrees to quaternion
// this is the default setting in the Axis Neuron server.
static quat eulerToQuat(const vec3& e) {
    // euler.x and euler.y are swaped, WTF.
    return (glm::angleAxis(e.x * RADIANS_PER_DEGREE, Vectors::UNIT_Y) *
            glm::angleAxis(e.y * RADIANS_PER_DEGREE, Vectors::UNIT_X) *
            glm::angleAxis(e.z * RADIANS_PER_DEGREE, Vectors::UNIT_Z));
}

//
// neuronDataReader SDK callback functions
//

// NOTE: must be thread-safe
void FrameDataReceivedCallback(void* context, SOCKET_REF sender, BvhDataHeaderEx* header, float* data) {

    auto neuronPlugin = reinterpret_cast<NeuronPlugin*>(context);

    // version 1.0
    if (header->DataVersion.Major == 1 && header->DataVersion.Minor == 0) {

        // skip reference joint if present
        if (header->WithReference && header->WithDisp) {
            data += 6;
        } else if (header->WithReference && !header->WithDisp) {
            data += 3;
        }

        if (header->WithDisp) {
            // enter mutex
            std::lock_guard<std::mutex> guard(neuronPlugin->_jointsMutex);

            //
            // Data is 6 floats per joint: 3 position values, 3 rotation euler angles (degrees)
            //

            // resize vector if necessary
            const size_t NUM_FLOATS_PER_JOINT = 6;
            const size_t NUM_JOINTS = header->DataCount / NUM_FLOATS_PER_JOINT;
            if (neuronPlugin->_joints.size() != NUM_JOINTS) {
                neuronPlugin->_joints.resize(NUM_JOINTS, { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } });
            }

            assert(sizeof(NeuronPlugin::NeuronJoint) == (NUM_FLOATS_PER_JOINT * sizeof(float)));

            // copy the data
            memcpy(&(neuronPlugin->_joints[0]), data, sizeof(NeuronPlugin::NeuronJoint) * NUM_JOINTS);
        }
    } else {
        static bool ONCE = false;
        if (!ONCE) {
            qCCritical(inputplugins) << "NeuronPlugin: bad frame version number, expected 1.0";
            ONCE = true;
        }
    }
}

// I can't even get the SDK to send me a callback.
// BRCommandFetchAvatarDataFromServer & BRRegisterAutoSyncParmeter [sic] don't seem to work.
// So this is totally untested.
// NOTE: must be thread-safe
static void CommandDataReceivedCallback(void* context, SOCKET_REF sender, CommandPack* pack, void* data) {

    DATA_VER version;
    version._VersionMask = pack->DataVersion;
    if (version.Major == 1 && version.Minor == 0) {
        const char* str = "Unknown";
        switch (pack->CommandId) {
        case Cmd_BoneSize:                // Id can be used to request bone size from server or register avatar name command.
            str = "BoneSize";
            break;
        case Cmd_AvatarName:              // Id can be used to request avatar name from server or register avatar name command.
            str = "AvatarName";
            break;
        case Cmd_FaceDirection:           // Id used to request face direction from server
            str = "FaceDirection";
            break;
        case Cmd_DataFrequency:           // Id can be used to request data frequency from server or register data frequency command.
            str = "DataFrequency";
            break;
        case Cmd_BvhInheritanceTxt:       // Id can be used to request bvh header txt from server or register bvh header txt command.
            str = "BvhInheritanceTxt";
            break;
        case Cmd_AvatarCount:             // Id can be used to request avatar count from server or register avatar count command.
            str = "AvatarCount";
            break;
        case Cmd_CombinationMode:         // Id can be used to request combination mode from server or register combination mode command.
            str = "CombinationMode";
            break;
        case Cmd_RegisterEvent:           // Id can be used to register event.
            str = "RegisterEvent";
            break;
        case Cmd_UnRegisterEvent:         // Id can be used to unregister event.
            str = "UnRegisterEvent";
            break;
        }
        qCDebug(inputplugins) << "NeuronPlugin: command data received CommandID = " << str;
    } else {
        static bool ONCE = false;
        if (!ONCE) {
            qCCritical(inputplugins) << "NeuronPlugin: bad command version number, expected 1.0";
            ONCE = true;
        }
    }
}

// NOTE: must be thread-safe
static void SocketStatusChangedCallback(void* context, SOCKET_REF sender, SocketStatus status, char* message) {
    // just dump to log, later we might want to pop up a connection lost dialog or attempt to reconnect.
    qCDebug(inputplugins) << "NeuronPlugin: socket status = " << message;
}

//
// NeuronPlugin
//

void NeuronPlugin::init() {
    loadSettings();

    auto preferences = DependencyManager::get<Preferences>();
    static const QString NEURON_PLUGIN { "Perception Neuron" };
    {
        auto getter = [this]()->bool { return _enabled; };
        auto setter = [this](bool value) { _enabled = value; saveSettings(); };
        auto preference = new CheckPreference(NEURON_PLUGIN, "Enabled", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = [this]()->QString { return _serverAddress; };
        auto setter = [this](const QString& value) { _serverAddress = value; saveSettings(); };
        auto preference = new EditPreference(NEURON_PLUGIN, "Server Address", getter, setter);
        preference->setPlaceholderText("");
        preferences->addPreference(preference);
    }
    {
        static const int MIN_PORT_NUMBER { 0 };
        static const int MAX_PORT_NUMBER { 65535 };

        auto getter = [this]()->float { return (float)_serverPort; };
        auto setter = [this](float value) { _serverPort = (int)value; saveSettings(); };
        auto preference = new SpinnerPreference(NEURON_PLUGIN, "Server Port", getter, setter);

        preference->setMin(MIN_PORT_NUMBER);
        preference->setMax(MAX_PORT_NUMBER);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
}

bool NeuronPlugin::isSupported() const {
    // Because it's a client/server network architecture, we can't tell
    // if the neuron is actually connected until we connect to the server.
    return true;
}

bool NeuronPlugin::activate() {
    InputPlugin::activate();

    loadSettings();

    if (_enabled) {

        // register with userInputMapper
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->registerDevice(_inputDevice);

        // register c-style callbacks
        BRRegisterFrameDataCallback((void*)this, FrameDataReceivedCallback);
        BRRegisterCommandDataCallback((void*)this, CommandDataReceivedCallback);
        BRRegisterSocketStatusCallback((void*)this, SocketStatusChangedCallback);

        // convert _serverAddress into a c-string.
        QByteArray serverAddressByteArray = _serverAddress.toUtf8();
        char* cstr = serverAddressByteArray.data();

        _socketRef = BRConnectTo(cstr, _serverPort);
        if (!_socketRef) {
            qCWarning(inputplugins) << "NeuronPlugin: error connecting to " << _serverAddress << ":" << _serverPort << ", error = " << BRGetLastErrorMessage();
            return false;
        } else {
            qCDebug(inputplugins) << "NeuronPlugin: success connecting to " << _serverAddress << ":" << _serverPort;

            emit deviceConnected(getName());

            BRRegisterAutoSyncParmeter(_socketRef, Cmd_CombinationMode);
            return true;
        }
    } else {
        return false;
    }
}

void NeuronPlugin::deactivate() {
    // unregister from userInputMapper
    if (_inputDevice->_deviceID != controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->removeDevice(_inputDevice->_deviceID);
    }

    if (_socketRef) {
        BRUnregisterAutoSyncParmeter(_socketRef, Cmd_CombinationMode);
        BRCloseSocket(_socketRef);
    }

    InputPlugin::deactivate();
    saveSettings();
}

void NeuronPlugin::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    if (!_enabled) {
        return;
    }

    std::vector<NeuronJoint> joints;
    {
        // lock and copy
        std::lock_guard<std::mutex> guard(_jointsMutex);
        joints = _joints;
    }

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData, joints, _prevJoints);
    });

    _prevJoints = joints;
}

void NeuronPlugin::saveSettings() const {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.setValue(QString("enabled"), _enabled);
        settings.setValue(QString("serverAddress"), _serverAddress);
        settings.setValue(QString("serverPort"), _serverPort);
    }
    settings.endGroup();
}

void NeuronPlugin::loadSettings() {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        // enabled
        _enabled = settings.value("enabled", QVariant(DEFAULT_ENABLED)).toBool();

        // serverAddress
        _serverAddress = settings.value("serverAddress", QVariant(DEFAULT_SERVER_ADDRESS)).toString();

        // serverPort
        bool canConvertPortToInt = false;
        int port = settings.value("serverPort", QVariant(DEFAULT_SERVER_PORT)).toInt(&canConvertPortToInt);
        if (canConvertPortToInt) {
            _serverPort = port;
        }
    }
    settings.endGroup();
}

//
// InputDevice
//

controller::Input::NamedVector NeuronPlugin::InputDevice::getAvailableInputs() const {
    static controller::Input::NamedVector availableInputs;
    if (availableInputs.size() == 0) {
        for (int i = 0; i < controller::NUM_STANDARD_POSES; i++) {
            auto channel = static_cast<controller::StandardPoseChannel>(i);
            availableInputs.push_back(makePair(channel, controllerJointName(channel)));
        }
    };
    return availableInputs;
}

QString NeuronPlugin::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/neuron.json";
    return MAPPING_JSON;
}

void NeuronPlugin::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, const std::vector<NeuronPlugin::NeuronJoint>& joints, const std::vector<NeuronPlugin::NeuronJoint>& prevJoints) {
    for (size_t i = 0; i < joints.size(); i++) {
        int poseIndex = neuronJointIndexToPoseIndex((NeuronJointIndex)i);
        glm::vec3 linearVel, angularVel;
        const glm::vec3& pos = joints[i].pos;
        const glm::vec3& rotEuler = joints[i].euler;

        if (Vectors::ZERO == pos && Vectors::ZERO == rotEuler) {
            _poseStateMap[poseIndex] = controller::Pose();
            continue;
        }

        glm::quat rot = eulerToQuat(rotEuler);
        if (i < prevJoints.size()) {
            linearVel = (pos - (prevJoints[i].pos * METERS_PER_CENTIMETER)) / deltaTime;  // m/s
            // quat log imaginary part points along the axis of rotation, with length of one half the angle of rotation.
            glm::quat d = glm::log(rot * glm::inverse(eulerToQuat(prevJoints[i].euler)));
            angularVel = glm::vec3(d.x, d.y, d.z) / (0.5f * deltaTime); // radians/s
        }
        _poseStateMap[poseIndex] = controller::Pose(pos, rot, linearVel, angularVel);
    }

    _poseStateMap[controller::RIGHT_HAND_THUMB1] = controller::Pose(rightHandThumb1DefaultAbsTranslation, glm::quat(), glm::vec3(), glm::vec3());
    _poseStateMap[controller::LEFT_HAND_THUMB1] = controller::Pose(leftHandThumb1DefaultAbsTranslation, glm::quat(), glm::vec3(), glm::vec3());
}

