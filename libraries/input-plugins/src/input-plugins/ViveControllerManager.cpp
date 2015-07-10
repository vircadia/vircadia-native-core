//
//  ViveControllerManager.cpp
//  interface/src/devices
//
//  Created by Sam Gondelman on 6/29/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ViveControllerManager.h"

#include <PerfStat.h>

#include "GeometryCache.h"
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <DeferredLightingEffect.h>
#include <display-plugins\openvr\OpenVrHelpers.h>
#include "NumericalConstants.h"
#include "UserActivityLogger.h"

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

const QString ViveControllerManager::NAME("OpenVR (Vive) Hand Controllers");

extern vr::IVRSystem *_hmd;
extern vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
extern mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];

const unsigned int LEFT_MASK = 0U;
const unsigned int RIGHT_MASK = 1U;

const uint64_t VR_BUTTON_A = 1U << 1;
const uint64_t VR_GRIP_BUTTON = 1U << 2;
const uint64_t VR_TRACKPAD_BUTTON = 1ULL << 32;
const uint64_t VR_TRIGGER_BUTTON = 1ULL << 33;

const unsigned int BUTTON_A = 1U << 1;
const unsigned int GRIP_BUTTON = 1U << 2;
const unsigned int TRACKPAD_BUTTON = 1U << 3;
const unsigned int TRIGGER_BUTTON = 1U << 4;

const QString CONTROLLER_MODEL_STRING = "vr_controller_05_wireless_b";

ViveControllerManager& ViveControllerManager::getInstance() {
    static ViveControllerManager sharedInstance;
    return sharedInstance;
}

ViveControllerManager::ViveControllerManager() :
    _isInitialized(false),
    _isEnabled(true),
    _trackedControllers(0),
    _modelLoaded(false)
{
    
}

ViveControllerManager::~ViveControllerManager() {

}

const QString& ViveControllerManager::getName() const {
    return NAME;
}

bool ViveControllerManager::isSupported() const {
    return vr::VR_IsHmdPresent();
}

void ViveControllerManager::init() {
    ;
}

void ViveControllerManager::deinit() {
    ;
}

void ViveControllerManager::activate(PluginContainer * container) {
    activate();
}

/// Called when a plugin is no longer being used.  May be called multiple times.
void ViveControllerManager::deactivate() {
    ;
}

/**
 * Called by the application during it's idle phase.  If the plugin needs to do
 * CPU intensive work, it should launch a thread for that, rather than trying to
 * do long operations in the idle call
 */
void ViveControllerManager::idle() {
    update();
}

void ViveControllerManager::activate() {
    if (!_hmd) {
        vr::HmdError eError = vr::HmdError_None;
        _hmd = vr::VR_Init(&eError);
        Q_ASSERT(eError == vr::HmdError_None);
    }
    Q_ASSERT(_hmd);

    vr::RenderModel_t model;
    if (!_hmd->LoadRenderModel(CONTROLLER_MODEL_STRING.toStdString().c_str(), &model)) {
        qDebug("Unable to load render model %s\n", CONTROLLER_MODEL_STRING);
    } else {
        model::Mesh* mesh = new model::Mesh();
        model::MeshPointer meshPtr(mesh);
        _modelGeometry.setMesh(meshPtr);

        auto indexBuffer = new gpu::Buffer(3 * model.unTriangleCount * sizeof(uint16_t), (gpu::Byte*)model.rIndexData);
        auto indexBufferPtr = gpu::BufferPointer(indexBuffer);
        auto indexBufferView = new gpu::BufferView(indexBufferPtr, gpu::Element(gpu::SCALAR, gpu::UINT16, gpu::RAW));
        mesh->setIndexBuffer(*indexBufferView);

        auto vertexBuffer = new gpu::Buffer(model.unVertexCount * sizeof(vr::RenderModel_Vertex_t),
            (gpu::Byte*)model.rVertexData);
        auto vertexBufferPtr = gpu::BufferPointer(vertexBuffer);
        auto vertexBufferView = new gpu::BufferView(vertexBufferPtr,
            0,
            vertexBufferPtr->getSize() - sizeof(float) * 3,
            sizeof(vr::RenderModel_Vertex_t),
            gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW));
        mesh->setVertexBuffer(*vertexBufferView);
        mesh->addAttribute(gpu::Stream::NORMAL,
            gpu::BufferView(vertexBufferPtr,
            sizeof(float) * 3,
            vertexBufferPtr->getSize() - sizeof(float) * 3,
            sizeof(vr::RenderModel_Vertex_t),
            gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW)));
        //mesh->addAttribute(gpu::Stream::TEXCOORD,
        //    gpu::BufferView(vertexBufferPtr,
        //    2 * sizeof(float) * 3,
        //    vertexBufferPtr->getSize() - sizeof(float) * 3,
        //    sizeof(vr::RenderModel_Vertex_t),
        //    gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::RAW)));

        _modelLoaded = true;
    }

    _isInitialized = true;
}

void ViveControllerManager::render(RenderArgs* args) {
    PerformanceTimer perfTimer("ViveControllerManager::render");

    if (_modelLoaded) {
        UserInputMapper::PoseValue leftHand = _poseStateMap[makeInput(JointChannel::LEFT_HAND).getChannel()];
        UserInputMapper::PoseValue rightHand = _poseStateMap[makeInput(JointChannel::RIGHT_HAND).getChannel()];

        gpu::Batch batch;
        auto geometryCache = DependencyManager::get<GeometryCache>();
        geometryCache->useSimpleDrawPipeline(batch);
        batch.setProjectionTransform(mat4());

        if (leftHand.isValid()) {
            renderHand(leftHand, batch);
        }
        if (rightHand.isValid()) {
            renderHand(rightHand, batch);
        }

        args->_context->syncCache();
        args->_context->render(batch);
    }
}

void ViveControllerManager::renderHand(UserInputMapper::PoseValue pose, gpu::Batch& batch) {
    Transform transform;
    transform.setTranslation(pose.getTranslation());
    transform.setRotation(pose.getRotation());

    auto mesh = _modelGeometry.getMesh();
    DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch);
    batch.setModelTransform(transform);
    batch.setInputFormat(mesh->getVertexFormat());
    batch.setInputBuffer(gpu::Stream::POSITION, mesh->getVertexBuffer());
    batch.setInputBuffer(gpu::Stream::NORMAL,
        mesh->getVertexBuffer()._buffer,
        sizeof(float) * 3,
        mesh->getVertexBuffer()._stride);
    batch.setIndexBuffer(gpu::UINT16, mesh->getIndexBuffer()._buffer, 0);
    batch.drawIndexed(gpu::TRIANGLES, mesh->getNumIndices(), 0);
}

void ViveControllerManager::update() {
    if (!_hmd) {
        return;
    }
    _buttonPressedMap.clear();
    if (_isInitialized && _isEnabled) {
        
        PerformanceTimer perfTimer("ViveControllerManager::update");

        int numTrackedControllers = 0;
        
        for (vr::TrackedDeviceIndex_t device = vr::k_unTrackedDeviceIndex_Hmd + 1;
             device < vr::k_unMaxTrackedDeviceCount && numTrackedControllers < 2; ++device) {
            
            if (!_hmd->IsTrackedDeviceConnected(device)) {
                continue;
            }
            
            if(_hmd->GetTrackedDeviceClass(device) != vr::TrackedDeviceClass_Controller) {
                continue;
            }

            if (!_trackedDevicePose[device].bPoseIsValid) {
                continue;
            }
            
            numTrackedControllers++;
            
            const mat4& mat = _trackedDevicePoseMat4[device];
                        
            handlePoseEvent(mat, numTrackedControllers - 1);
            
            // handle inputs
            vr::VRControllerState_t controllerState = vr::VRControllerState_t();
            if (_hmd->GetControllerState(device, &controllerState)) {
                //qDebug() << (numTrackedControllers == 1 ? "Left: " : "Right: ");
                //qDebug() << "Trackpad: " << controllerState.rAxis[0].x << " " << controllerState.rAxis[0].y;
                //qDebug() << "Trigger: " << controllerState.rAxis[1].x << " " << controllerState.rAxis[1].y;
                handleButtonEvent(controllerState.ulButtonPressed, numTrackedControllers - 1);
                for (int i = 0; i < 5; i++) {
                    handleAxisEvent(Axis(i), controllerState.rAxis[i].x, controllerState.rAxis[i].y, numTrackedControllers - 1);
                }
            }
        }
        
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        
        if (numTrackedControllers == 0) {
            if (_deviceID != 0) {
                userInputMapper->removeDevice(_deviceID);
                _deviceID = 0;
                _poseStateMap.clear();
            }
        }
        
        if (_trackedControllers == 0 && numTrackedControllers > 0) {
            registerToUserInputMapper(*userInputMapper);
            assignDefaultInputMapping(*userInputMapper);
            UserActivityLogger::getInstance().connectedDevice("spatial_controller", "steamVR");
        }
        
        _trackedControllers = numTrackedControllers;
    }
}

void ViveControllerManager::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void ViveControllerManager::handleAxisEvent(Axis axis, float x, float y, int index) {
    if (axis == TRACKPAD_AXIS) {
        _axisStateMap[makeInput(AXIS_Y_POS, index).getChannel()] = (y > 0.0f) ? y : 0.0f;
        _axisStateMap[makeInput(AXIS_Y_NEG, index).getChannel()] = (y < 0.0f) ? -y : 0.0f;
        _axisStateMap[makeInput(AXIS_X_POS, index).getChannel()] = (x > 0.0f) ? x : 0.0f;
        _axisStateMap[makeInput(AXIS_X_NEG, index).getChannel()] = (x < 0.0f) ? -x : 0.0f;
    } else if (axis == TRIGGER_AXIS) {
        _axisStateMap[makeInput(BACK_TRIGGER, index).getChannel()] = x;
    }
}

void ViveControllerManager::handleButtonEvent(uint64_t buttons, int index) {
    if (buttons & VR_BUTTON_A) {
        _buttonPressedMap.insert(makeInput(BUTTON_A, index).getChannel());
    }
    if (buttons & VR_GRIP_BUTTON) {
        _buttonPressedMap.insert(makeInput(GRIP_BUTTON, index).getChannel());
    }
    if (buttons & VR_TRACKPAD_BUTTON) {
        _buttonPressedMap.insert(makeInput(TRACKPAD_BUTTON, index).getChannel());
    }
    if (buttons & VR_TRIGGER_BUTTON) {
        _buttonPressedMap.insert(makeInput(TRIGGER_BUTTON, index).getChannel());
    }
}

void ViveControllerManager::handlePoseEvent(const mat4& mat, int index) {
    glm::vec3 position = extractTranslation(mat);
    glm::quat rotation = glm::quat_cast(mat);

    // Flip the rotation appropriately for each hand
    if (index == LEFT_HAND) {
        rotation = rotation * glm::angleAxis(PI, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::angleAxis(PI_OVER_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
    } else if (index == RIGHT_HAND) {
        rotation = rotation * glm::angleAxis(PI, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::angleAxis(PI + PI_OVER_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
    }

    const float CONTROLLER_LENGTH_OFFSET = 0.1f;
    position += rotation * glm::vec3(0, 0, -CONTROLLER_LENGTH_OFFSET);

    _poseStateMap[makeInput(JointChannel(index)).getChannel()] = UserInputMapper::PoseValue(position, rotation);
}

void ViveControllerManager::registerToUserInputMapper(UserInputMapper& mapper) {
    // Grab the current free device ID
    _deviceID = mapper.getFreeDeviceID();
    
    auto proxy = UserInputMapper::DeviceProxy::Pointer(new UserInputMapper::DeviceProxy("SteamVR Controller"));
    proxy->getButton = [this] (const UserInputMapper::Input& input, int timestamp) -> bool { return this->getButton(input.getChannel()); };
    proxy->getAxis = [this] (const UserInputMapper::Input& input, int timestamp) -> float { return this->getAxis(input.getChannel()); };
    proxy->getPose = [this](const UserInputMapper::Input& input, int timestamp) -> UserInputMapper::PoseValue { return this->getPose(input.getChannel()); };
    proxy->getAvailabeInputs = [this] () -> QVector<UserInputMapper::InputPair> {
        QVector<UserInputMapper::InputPair> availableInputs;
        availableInputs.append(UserInputMapper::InputPair(makeInput(LEFT_HAND), "Left Hand"));
        
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_A, 0), "Left Button A"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(GRIP_BUTTON, 0), "Left Grip Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(TRACKPAD_BUTTON, 0), "Left Trackpad Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(TRIGGER_BUTTON, 0), "Left Trigger Button"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_POS, 0), "Left Trackpad Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_NEG, 0), "Left Trackpad Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_POS, 0), "Left Trackpad Right"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_NEG, 0), "Left Trackpad Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BACK_TRIGGER, 0), "Left Back Trigger"));

        
        availableInputs.append(UserInputMapper::InputPair(makeInput(RIGHT_HAND), "Right Hand"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_A, 1), "Right Button A"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(GRIP_BUTTON, 1), "Right Grip Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(TRACKPAD_BUTTON, 1), "Right Trackpad Button"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(TRIGGER_BUTTON, 1), "Right Trigger Button"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_POS, 1), "Right Trackpad Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_NEG, 1), "Right Trackpad Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_POS, 1), "Right Trackpad Right"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_NEG, 1), "Right Trackpad Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BACK_TRIGGER, 1), "Right Back Trigger"));
        
        return availableInputs;
    };
    proxy->resetDeviceBindings = [this, &mapper] () -> bool {
        mapper.removeAllInputChannelsForDevice(_deviceID);
        this->assignDefaultInputMapping(mapper);
        return true;
    };
    mapper.registerDevice(_deviceID, proxy);
}

void ViveControllerManager::assignDefaultInputMapping(UserInputMapper& mapper) {
    const float JOYSTICK_MOVE_SPEED = 1.0f;
    const float BOOM_SPEED = 0.1f;
    
    // Left Trackpad: Movement, strafing
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(AXIS_Y_POS, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(AXIS_Y_NEG, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(AXIS_X_POS, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(AXIS_X_NEG, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);

    // Right Trackpad: Vertical movement, zooming
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(AXIS_Y_POS, 1), makeInput(TRACKPAD_BUTTON, 1), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(AXIS_Y_NEG, 1), makeInput(TRACKPAD_BUTTON, 1), JOYSTICK_MOVE_SPEED);
    
    // Buttons
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(GRIP_BUTTON, 0));
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(GRIP_BUTTON, 1));
    
    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(BUTTON_A, 0));
    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(BUTTON_A, 1));

    mapper.addInputChannel(UserInputMapper::LEFT_HAND_CLICK, makeInput(TRIGGER_BUTTON, 0));
    mapper.addInputChannel(UserInputMapper::RIGHT_HAND_CLICK, makeInput(TRIGGER_BUTTON, 1));
    
    // Hands
    mapper.addInputChannel(UserInputMapper::LEFT_HAND, makeInput(LEFT_HAND));
    mapper.addInputChannel(UserInputMapper::RIGHT_HAND, makeInput(RIGHT_HAND));
}

float ViveControllerManager::getButton(int channel) const {
    if (!_buttonPressedMap.empty()) {
        if (_buttonPressedMap.find(channel) != _buttonPressedMap.end()) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    }
    return 0.0f;
}

float ViveControllerManager::getAxis(int channel) const {
    auto axis = _axisStateMap.find(channel);
    if (axis != _axisStateMap.end()) {
        return (*axis).second;
    } else {
        return 0.0f;
    }
}

UserInputMapper::PoseValue ViveControllerManager::getPose(int channel) const {
    auto pose = _poseStateMap.find(channel);
    if (pose != _poseStateMap.end()) {
        return (*pose).second;
    } else {
        return UserInputMapper::PoseValue();
    }
}

UserInputMapper::Input ViveControllerManager::makeInput(unsigned int button, int index) {
    return UserInputMapper::Input(_deviceID, button | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input ViveControllerManager::makeInput(ViveControllerManager::JoystickAxisChannel axis, int index) {
    return UserInputMapper::Input(_deviceID, axis | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::AXIS);
}

UserInputMapper::Input ViveControllerManager::makeInput(JointChannel joint) {
    return UserInputMapper::Input(_deviceID, joint, UserInputMapper::ChannelType::POSE);
}
