//
//  NeuronPlugin.h
//  input-plugins/src/input-plugins
//
//  Created by Anthony Thibault on 12/18/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NeuronPlugin.h"

#include <QLoggingCategory>
#include <PathUtils.h>
#include <DebugDraw.h>
#include <cassert>
#include <NumericalConstants.h>

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

#define __OS_XUN__ 1
#define BOOL int
#include <NeuronDataReader.h>

const QString NeuronPlugin::NAME = "Neuron";
const QString NeuronPlugin::NEURON_ID_STRING = "Perception Neuron";

enum JointIndex {
    HipsPosition = 0,
    Hips,
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
    LeftHandPinky3
};

bool NeuronPlugin::isSupported() const {
    // Because it's a client/server network architecture, we can't tell
    // if the neuron is actually connected until we connect to the server.
    return true;
}

// NOTE: must be thread-safe
void FrameDataReceivedCallback(void* context, SOCKET_REF sender, BvhDataHeaderEx* header, float* data) {
    qCDebug(inputplugins) << "NeuronPlugin: received frame data, DataCount = " << header->DataCount;

    auto neuronPlugin = reinterpret_cast<NeuronPlugin*>(context);
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
}

// NOTE: must be thread-safe
static void CommandDataReceivedCallback(void* context, SOCKET_REF sender, CommandPack* pack, void* data) {

}

// NOTE: must be thread-safe
static void SocketStatusChangedCallback(void* context, SOCKET_REF sender, SocketStatus status, char* message) {
    qCDebug(inputplugins) << "NeuronPlugin: socket status = " << message;
}

void NeuronPlugin::activate() {
    InputPlugin::activate();
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
    }
    qCDebug(inputplugins) << "NeuronPlugin: success connecting to " << _serverAddress.c_str() << ":" << _serverPort;
}

void NeuronPlugin::deactivate() {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::deactivate";

    if (_socketRef) {
        BRCloseSocket(_socketRef);
    }
    InputPlugin::deactivate();
}

// convert between euler in degrees to quaternion
static quat eulerToQuat(vec3 euler) {
    return (glm::angleAxis(euler.y * RADIANS_PER_DEGREE, Vectors::UNIT_Y) *
            glm::angleAxis(euler.x * RADIANS_PER_DEGREE, Vectors::UNIT_X) *
            glm::angleAxis(euler.z * RADIANS_PER_DEGREE, Vectors::UNIT_Z));
}

void NeuronPlugin::pluginUpdate(float deltaTime, bool jointsCaptured) {

    std::vector<NeuronJoint> joints;
    // copy the shared data
    {
        std::lock_guard<std::mutex> guard(_jointsMutex);
        joints = _joints;
    }

    DebugDraw::getInstance().addMyAvatarMarker("LEFT_FOOT",
                                               eulerToQuat(joints[6].rot),
                                               joints[6].pos / 100.0f,
                                               glm::vec4(1));

    _inputDevice->update(deltaTime, jointsCaptured);
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

controller::Input::NamedVector NeuronPlugin::InputDevice::getAvailableInputs() const {
    // TODO:
    static const controller::Input::NamedVector availableInputs {
        makePair(controller::LEFT_HAND, "LeftHand"),
        makePair(controller::RIGHT_HAND, "RightHand")
    };
    return availableInputs;
}

QString NeuronPlugin::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/neuron.json";
    return MAPPING_JSON;
}

void NeuronPlugin::InputDevice::update(float deltaTime, bool jointsCaptured) {

}

void NeuronPlugin::InputDevice::focusOutEvent() {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::InputDevice::focusOutEvent";
}
