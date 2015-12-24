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

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

#define __OS_XUN__ 1
#define BOOL int
#include <NeuronDataReader.h>

const QString NeuronPlugin::NAME = "Neuron";
const QString NeuronPlugin::NEURON_ID_STRING = "Perception Neuron";

// This matches controller::StandardPoseChannel
enum JointIndex {
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

bool NeuronPlugin::isSupported() const {
    // Because it's a client/server network architecture, we can't tell
    // if the neuron is actually connected until we connect to the server.
    return true;
}

// NOTE: must be thread-safe
void FrameDataReceivedCallback(void* context, SOCKET_REF sender, BvhDataHeaderEx* header, float* data) {

    auto neuronPlugin = reinterpret_cast<NeuronPlugin*>(context);

    // version 1.0
    if (header->DataVersion.Major == 1 && header->DataVersion.Minor == 0) {

        std::lock_guard<std::mutex> guard(neuronPlugin->_jointsMutex);

        // Data is 6 floats: 3 position values, 3 rotation euler angles (degrees)

        // resize vector if necessary
        const size_t NUM_FLOATS_PER_JOINT = 6;
        const size_t NUM_JOINTS = header->DataCount / NUM_FLOATS_PER_JOINT;
        if (neuronPlugin->_joints.size() != NUM_JOINTS) {
            neuronPlugin->_joints.resize(NUM_JOINTS, { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } });
        }

        assert(sizeof(NeuronPlugin::NeuronJoint) == (NUM_FLOATS_PER_JOINT * sizeof(float)));

        // copy the data
        memcpy(&(neuronPlugin->_joints[0]), data, sizeof(NeuronPlugin::NeuronJoint) * NUM_JOINTS);

    } else {
        static bool ONCE = false;
        if (!ONCE) {
            qCCritical(inputplugins) << "NeuronPlugin: bad frame version number, expected 1.0";
            ONCE = true;
        }
    }
}

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
    qCDebug(inputplugins) << "NeuronPlugin: socket status = " << message;
}

void NeuronPlugin::activate() {
    InputPlugin::activate();

    // register with userInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(_inputDevice);

    qCDebug(inputplugins) << "NeuronPlugin::activate";

    // register c-style callbacks
    BRRegisterFrameDataCallback((void*)this, FrameDataReceivedCallback);
    BRRegisterCommandDataCallback((void*)this, CommandDataReceivedCallback);
    BRRegisterSocketStatusCallback((void*)this, SocketStatusChangedCallback);

    // TODO: pull these from prefs!
    _serverAddress = "localhost";
    _serverPort = 7001;
    _socketRef = BRConnectTo((char*)_serverAddress.c_str(), _serverPort);
    if (!_socketRef) {
        // error
        qCCritical(inputplugins) << "NeuronPlugin: error connecting to " << _serverAddress.c_str() << ":" << _serverPort << "error = " << BRGetLastErrorMessage();
    } else {
        qCDebug(inputplugins) << "NeuronPlugin: success connecting to " << _serverAddress.c_str() << ":" << _serverPort;
    }
}

void NeuronPlugin::deactivate() {
    qCDebug(inputplugins) << "NeuronPlugin::deactivate";

    // unregister from userInputMapper
    if (_inputDevice->_deviceID != controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->removeDevice(_inputDevice->_deviceID);
    }

    if (_socketRef) {
        BRCloseSocket(_socketRef);
    }

    InputPlugin::deactivate();
}

// convert between euler in degrees to quaternion
static quat eulerToQuat(vec3 euler) {
    // euler.x and euler.y are swaped (thanks NOMICOM!)
    glm::vec3 e = glm::vec3(euler.y, euler.x, euler.z) * RADIANS_PER_DEGREE;
    return (glm::angleAxis(e.y, Vectors::UNIT_Y) * glm::angleAxis(e.x, Vectors::UNIT_X) * glm::angleAxis(e.z, Vectors::UNIT_Z));
}

void NeuronPlugin::pluginUpdate(float deltaTime, bool jointsCaptured) {

    std::vector<NeuronJoint> joints;
    // copy the shared data
    {
        std::lock_guard<std::mutex> guard(_jointsMutex);
        joints = _joints;
    }

    /*
    DebugDraw::getInstance().addMyAvatarMarker("LEFT_FOOT",
                                               eulerToQuat(joints[6].euler),
                                               joints[6].pos / 100.0f,
                                               glm::vec4(1));
    */
    _inputDevice->update(deltaTime, joints, _prevJoints);
    _prevJoints = joints;
}

void NeuronPlugin::saveSettings() const {
    InputPlugin::saveSettings();
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::saveSettings";
}

void NeuronPlugin::loadSettings() {
    InputPlugin::loadSettings();
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::loadSettings";
}

//
// InputDevice
//

static controller::StandardPoseChannel neuronJointIndexToPoseIndex(JointIndex i) {
    // Currently they are the same.
    // but that won't always be the case...
    return (controller::StandardPoseChannel)i;
}

static const char* neuronJointName(JointIndex i) {
    switch (i) {
    case Hips: return "Hips";
    case RightUpLeg: return "RightUpLeg";
    case RightLeg: return "RightLeg";
    case RightFoot: return "RightFoot";
    case LeftUpLeg: return "LeftUpLeg";
    case LeftLeg: return "LeftLeg";
    case LeftFoot: return "LeftFoot";
    case Spine: return "Spine";
    case Spine1: return "Spine1";
    case Spine2: return "Spine2";
    case Spine3: return "Spine3";
    case Neck: return "Neck";
    case Head: return "Head";
    case RightShoulder: return "RightShoulder";
    case RightArm: return "RightArm";
    case RightForeArm: return "RightForeArm";
    case RightHand: return "RightHand";
    case RightHandThumb1: return "RightHandThumb1";
    case RightHandThumb2: return "RightHandThumb2";
    case RightHandThumb3: return "RightHandThumb3";
    case RightInHandIndex: return "RightInHandIndex";
    case RightHandIndex1: return "RightHandIndex1";
    case RightHandIndex2: return "RightHandIndex2";
    case RightHandIndex3: return "RightHandIndex3";
    case RightInHandMiddle: return "RightInHandMiddle";
    case RightHandMiddle1: return "RightHandMiddle1";
    case RightHandMiddle2: return "RightHandMiddle2";
    case RightHandMiddle3: return "RightHandMiddle3";
    case RightInHandRing: return "RightInHandRing";
    case RightHandRing1: return "RightHandRing1";
    case RightHandRing2: return "RightHandRing2";
    case RightHandRing3: return "RightHandRing3";
    case RightInHandPinky: return "RightInHandPinky";
    case RightHandPinky1: return "RightHandPinky1";
    case RightHandPinky2: return "RightHandPinky2";
    case RightHandPinky3: return "RightHandPinky3";
    case LeftShoulder: return "LeftShoulder";
    case LeftArm: return "LeftArm";
    case LeftForeArm: return "LeftForeArm";
    case LeftHand: return "LeftHand";
    case LeftHandThumb1: return "LeftHandThumb1";
    case LeftHandThumb2: return "LeftHandThumb2";
    case LeftHandThumb3: return "LeftHandThumb3";
    case LeftInHandIndex: return "LeftInHandIndex";
    case LeftHandIndex1: return "LeftHandIndex1";
    case LeftHandIndex2: return "LeftHandIndex2";
    case LeftHandIndex3: return "LeftHandIndex3";
    case LeftInHandMiddle: return "LeftInHandMiddle";
    case LeftHandMiddle1: return "LeftHandMiddle1";
    case LeftHandMiddle2: return "LeftHandMiddle2";
    case LeftHandMiddle3: return "LeftHandMiddle3";
    case LeftInHandRing: return "LeftInHandRing";
    case LeftHandRing1: return "LeftHandRing1";
    case LeftHandRing2: return "LeftHandRing2";
    case LeftHandRing3: return "LeftHandRing3";
    case LeftInHandPinky: return "LeftInHandPinky";
    case LeftHandPinky1: return "LeftHandPinky1";
    case LeftHandPinky2: return "LeftHandPinky2";
    case LeftHandPinky3: return "LeftHandPinky3";
    default: return "???";
    }
}

controller::Input::NamedVector NeuronPlugin::InputDevice::getAvailableInputs() const {
    // TODO:
    static controller::Input::NamedVector availableInputs;

    if (availableInputs.size() == 0) {
        for (int i = 0; i < JointIndex::Size; i++) {
            availableInputs.push_back(makePair(neuronJointIndexToPoseIndex((JointIndex)i), neuronJointName((JointIndex)i)));
        }
    };
    return availableInputs;
}

QString NeuronPlugin::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/neuron.json";
    return MAPPING_JSON;
}

void NeuronPlugin::InputDevice::update(float deltaTime, const std::vector<NeuronPlugin::NeuronJoint>& joints, const std::vector<NeuronPlugin::NeuronJoint>& prevJoints) {
    for (int i = 0; i < joints.size(); i++) {
        int poseIndex = neuronJointIndexToPoseIndex((JointIndex)i);
        glm::vec3 linearVel, angularVel;
        glm::vec3 pos = (joints[i].pos * METERS_PER_CENTIMETER);
        glm::quat rot = eulerToQuat(joints[i].euler);
        if (i < prevJoints.size()) {
            linearVel = (pos - (prevJoints[i].pos * METERS_PER_CENTIMETER)) / deltaTime;
            // quat log imag part points along the axis of rotation, and it's length will be the half angle.
            glm::quat d = glm::log(rot * glm::inverse(eulerToQuat(prevJoints[i].euler)));
            angularVel = glm::vec3(d.x, d.y, d.z) / (0.5f * deltaTime);
        }
        _poseStateMap[poseIndex] = controller::Pose(pos, rot, linearVel, angularVel);

        // dump the first 5 joints
        if (i <= JointIndex::LeftFoot) {
            qCDebug(inputplugins) << neuronJointName((JointIndex)i);
            qCDebug(inputplugins) << "    trans = " << joints[i].pos;
            qCDebug(inputplugins) << "    euler = " << joints[i].euler;
        }
        /*
        if (glm::length(angularVel) > 0.5f) {
            qCDebug(inputplugins) << "Movement in joint" << i << neuronJointName((JointIndex)i);
        }
        */
    }
}

void NeuronPlugin::InputDevice::focusOutEvent() {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::InputDevice::focusOutEvent";
}
