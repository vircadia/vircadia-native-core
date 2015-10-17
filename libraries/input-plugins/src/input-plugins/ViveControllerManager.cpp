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

#include <PerfStat.h>

#include "GeometryCache.h"
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <DeferredLightingEffect.h>
#include <display-plugins/openvr/OpenVrHelpers.h>
#include "NumericalConstants.h"
#include <plugins/PluginContainer.h>
#include "UserActivityLogger.h"

#ifdef Q_OS_WIN
extern vr::IVRSystem* _hmd;
extern int hmdRefCount;
extern vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
extern mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];
#endif

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

const float CONTROLLER_LENGTH_OFFSET = 0.0762f;  // three inches
const QString CONTROLLER_MODEL_STRING = "vr_controller_05_wireless_b";

const QString ViveControllerManager::NAME = "OpenVR";

const QString MENU_PARENT = "Avatar";
const QString MENU_NAME = "Vive Controllers";
const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
const QString RENDER_CONTROLLERS = "Render Hand Controllers";

ViveControllerManager::ViveControllerManager() :
        InputDevice("SteamVR Controller"),
    _trackedControllers(0),
    _modelLoaded(false),
    _leftHandRenderID(0),
    _rightHandRenderID(0),
    _renderControllers(false)
{
    
}

bool ViveControllerManager::isSupported() const {
#ifdef Q_OS_WIN
    bool success = vr::VR_IsHmdPresent();
    if (success) {
        vr::HmdError eError = vr::HmdError_None;
        auto hmd = vr::VR_Init(&eError);
        success = (hmd != nullptr);
        vr::VR_Shutdown();
    }
    return success;
#else 
    return false;
#endif
}

void ViveControllerManager::activate() {
#ifdef Q_OS_WIN
    CONTAINER->addMenu(MENU_PATH);
    CONTAINER->addMenuItem(MENU_PATH, RENDER_CONTROLLERS,
        [this] (bool clicked) { this->setRenderControllers(clicked); },
        true, true);

    hmdRefCount++;
    if (!_hmd) {
        vr::HmdError eError = vr::HmdError_None;
        _hmd = vr::VR_Init(&eError);
        Q_ASSERT(eError == vr::HmdError_None);
    }
    Q_ASSERT(_hmd);

    vr::RenderModel_t model;
    if (!_hmd->LoadRenderModel(CONTROLLER_MODEL_STRING.toStdString().c_str(), &model)) {
        qDebug() << QString("Unable to load render model %1\n").arg(CONTROLLER_MODEL_STRING);
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
        //    vertexBufferPtr->getSize() - sizeof(float) * 2,
        //    sizeof(vr::RenderModel_Vertex_t),
        //    gpu::Element(gpu::VEC2, gpu::FLOAT, gpu::RAW)));
        
        gpu::Element formatGPU = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA);
        gpu::Element formatMip = gpu::Element(gpu::VEC4, gpu::UINT8, gpu::RGBA);
        _texture = gpu::TexturePointer(
            gpu::Texture::create2D(formatGPU, model.diffuseTexture.unWidth, model.diffuseTexture.unHeight,
            gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
        _texture->assignStoredMip(0, formatMip, model.diffuseTexture.unWidth * model.diffuseTexture.unHeight * 4 * sizeof(uint8_t), model.diffuseTexture.rubTextureMapData);
        _texture->autoGenerateMips(-1);

        _modelLoaded = true;
        _renderControllers = true;
    }
#endif
}

void ViveControllerManager::deactivate() {
#ifdef Q_OS_WIN
    CONTAINER->removeMenuItem(MENU_NAME, RENDER_CONTROLLERS);
    CONTAINER->removeMenu(MENU_PATH);

    hmdRefCount--;

    if (hmdRefCount == 0 && _hmd) {
        vr::VR_Shutdown();
        _hmd = nullptr;
    }
    _poseStateMap.clear();
#endif
}

void ViveControllerManager::updateRendering(RenderArgs* args, render::ScenePointer scene, render::PendingChanges pendingChanges) {
    PerformanceTimer perfTimer("ViveControllerManager::updateRendering");

    if (_modelLoaded) {
        //auto controllerPayload = new render::Payload<ViveControllerManager>(this);
        //auto controllerPayloadPointer = ViveControllerManager::PayloadPointer(controllerPayload);
        //if (_leftHandRenderID == 0) {
        //    _leftHandRenderID = scene->allocateID();
        //    pendingChanges.resetItem(_leftHandRenderID, controllerPayloadPointer);
        //}
        //pendingChanges.updateItem(_leftHandRenderID, );


        UserInputMapper::PoseValue leftHand = _poseStateMap[makeInput(JointChannel::LEFT_HAND).getChannel()];
        UserInputMapper::PoseValue rightHand = _poseStateMap[makeInput(JointChannel::RIGHT_HAND).getChannel()];

        gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
            auto geometryCache = DependencyManager::get<GeometryCache>();
            geometryCache->useSimpleDrawPipeline(batch);
            DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch, true);

            auto mesh = _modelGeometry.getMesh();
            batch.setInputFormat(mesh->getVertexFormat());
            //batch._glBindTexture(GL_TEXTURE_2D, _uexture);

            if (leftHand.isValid()) {
                renderHand(leftHand, batch, LEFT_HAND);
            }
            if (rightHand.isValid()) {
                renderHand(rightHand, batch, RIGHT_HAND);
            }
        });
    }
}

void ViveControllerManager::renderHand(UserInputMapper::PoseValue pose, gpu::Batch& batch, int index) {
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    Transform transform(userInputMapper->getSensorToWorldMat());
    transform.postTranslate(pose.getTranslation() + pose.getRotation() * glm::vec3(0, 0, CONTROLLER_LENGTH_OFFSET));

    int sign = index == LEFT_HAND ? 1 : -1;
    glm::quat rotation = pose.getRotation() * glm::angleAxis(PI, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::angleAxis(sign * PI_OVER_TWO, glm::vec3(0.0f, 0.0f, 1.0f));
    transform.postRotate(rotation);

    batch.setModelTransform(transform);

    auto mesh = _modelGeometry.getMesh();
    batch.setInputBuffer(gpu::Stream::POSITION, mesh->getVertexBuffer());
    batch.setInputBuffer(gpu::Stream::NORMAL,
        mesh->getVertexBuffer()._buffer,
        sizeof(float) * 3,
        mesh->getVertexBuffer()._stride);
    //batch.setInputBuffer(gpu::Stream::TEXCOORD,
    //    mesh->getVertexBuffer()._buffer,
    //    2 * 3 * sizeof(float),
    //    mesh->getVertexBuffer()._stride);
    batch.setIndexBuffer(gpu::UINT16, mesh->getIndexBuffer()._buffer, 0);
    batch.drawIndexed(gpu::TRIANGLES, mesh->getNumIndices(), 0);
}

void ViveControllerManager::update(float deltaTime, bool jointsCaptured) {
#ifdef Q_OS_WIN
    _poseStateMap.clear();

    // TODO: This shouldn't be necessary
    if (!_hmd) {
        return;
    }

    _buttonPressedMap.clear();

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
                  
        if (!jointsCaptured) {
            handlePoseEvent(mat, numTrackedControllers - 1);
        }
            
        // handle inputs
        vr::VRControllerState_t controllerState = vr::VRControllerState_t();
        if (_hmd->GetControllerState(device, &controllerState)) {
            //qDebug() << (numTrackedControllers == 1 ? "Left: " : "Right: ");
            //qDebug() << "Trackpad: " << controllerState.rAxis[0].x << " " << controllerState.rAxis[0].y;
            //qDebug() << "Trigger: " << controllerState.rAxis[1].x << " " << controllerState.rAxis[1].y;
            handleButtonEvent(controllerState.ulButtonPressed, numTrackedControllers - 1);
            for (int i = 0; i < vr::k_unControllerStateAxisCount; i++) {
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
#endif
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

    // When the sensor-to-world rotation is identity the coordinate axes look like this:
    //
    //                       user
    //                      forward
    //                         z
    //                         |
    //                        y|      user
    //      y                  o----x right
    //       o-----x         user
    //       |                up
    //       |
    //       z
    //
    //     Vive
    //

    // From ABOVE the hand canonical axes looks like this:
    //
    //      | | | |          y        | | | |
    //      | | | |          |        | | | |
    //      |     |          |        |     |
    //      |left | /  x---- +      \ |right|
    //      |     _/          z      \_     |
    //       |   |                     |   |
    //       |   |                     |   |
    //

    // So when the user is standing in Vive space facing the -zAxis with hands outstretched and palms down 
    // the rotation to align the Vive axes with those of the hands is:
    //
    //    QviveToHand = halfTurnAboutY * quaterTurnAboutX
   
    // Due to how the Vive controllers fit into the palm there is an offset that is different for each hand.
    // You can think of this offset as the inverse of the measured rotation when the hands are posed, such that
    // the combination (measurement * offset) is identity at this orientation.
    //
    //    Qoffset = glm::inverse(deltaRotation when hand is posed fingers forward, palm down)
    //
    // An approximate offset for the Vive can be obtained by inspection:
    //
    //    Qoffset = glm::inverse(glm::angleAxis(sign * PI/4.0f, zAxis) * glm::angleAxis(PI/2.0f, xAxis))
    //
    // So the full equation is:
    //
    //    Q = combinedMeasurement * viveToHand
    //
    //    Q = (deltaQ * QOffset) * (yFlip * quarterTurnAboutX)
    //
    //    Q = (deltaQ * inverse(deltaQForAlignedHand)) * (yFlip * quarterTurnAboutX)
   
    const glm::quat quarterX = glm::angleAxis(PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat yFlip = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f));
    float sign = (index == LEFT_HAND) ? -1.0f : 1.0f;
    const glm::quat signedQuaterZ = glm::angleAxis(sign * PI / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f)); 
    const glm::quat eighthX = glm::angleAxis(PI / 4.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat offset = glm::inverse(signedQuaterZ * eighthX);
    rotation = rotation * offset * yFlip * quarterX;

    position += rotation * glm::vec3(0, 0, -CONTROLLER_LENGTH_OFFSET);

    _poseStateMap[makeInput(JointChannel(index)).getChannel()] = UserInputMapper::PoseValue(position, rotation);
}

void ViveControllerManager::registerToUserInputMapper(UserInputMapper& mapper) {
    // Grab the current free device ID
    _deviceID = mapper.getFreeDeviceID();
    
    auto proxy = UserInputMapper::DeviceProxy::Pointer(new UserInputMapper::DeviceProxy(_name));
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
    
    // Left Trackpad: Movement, strafing
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(AXIS_Y_POS, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(AXIS_Y_NEG, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(AXIS_X_POS, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(AXIS_X_NEG, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);

    // Right Trackpad: Vertical movement, zooming
    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(AXIS_Y_POS, 1), makeInput(TRACKPAD_BUTTON, 1), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(AXIS_Y_NEG, 1), makeInput(TRACKPAD_BUTTON, 1), JOYSTICK_MOVE_SPEED);
    
    // Buttons
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(BUTTON_A, 0));
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(BUTTON_A, 1));
    
    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(GRIP_BUTTON, 0));
    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(GRIP_BUTTON, 1));

    mapper.addInputChannel(UserInputMapper::LEFT_HAND_CLICK, makeInput(BACK_TRIGGER, 0));
    mapper.addInputChannel(UserInputMapper::RIGHT_HAND_CLICK, makeInput(BACK_TRIGGER, 1));
    
    // Hands
    mapper.addInputChannel(UserInputMapper::LEFT_HAND, makeInput(LEFT_HAND));
    mapper.addInputChannel(UserInputMapper::RIGHT_HAND, makeInput(RIGHT_HAND));
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
