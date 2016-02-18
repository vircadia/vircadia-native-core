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
#include <PathUtils.h>
#include <GeometryCache.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <DeferredLightingEffect.h>
#include <NumericalConstants.h>
#include <plugins/PluginContainer.h>
#include <UserActivityLogger.h>

#include <controllers/UserInputMapper.h>

#include <controllers/StandardControls.h>

#include "OpenVrHelpers.h"

extern vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
extern mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];

vr::IVRSystem* acquireOpenVrSystem();
void releaseOpenVrSystem();


static const float CONTROLLER_LENGTH_OFFSET = 0.0762f;  // three inches
static const glm::vec3 CONTROLLER_OFFSET = glm::vec3(CONTROLLER_LENGTH_OFFSET / 2.0f,
                                                     CONTROLLER_LENGTH_OFFSET / 2.0f,
                                                     CONTROLLER_LENGTH_OFFSET * 2.0f);
static const char* CONTROLLER_MODEL_STRING = "vr_controller_05_wireless_b";

static const QString MENU_PARENT = "Avatar";
static const QString MENU_NAME = "Vive Controllers";
static const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
static const QString RENDER_CONTROLLERS = "Render Hand Controllers";

const QString ViveControllerManager::NAME = "OpenVR";

bool ViveControllerManager::isSupported() const {
    return vr::VR_IsHmdPresent();
}

void ViveControllerManager::activate() {
    InputPlugin::activate();
    _container->addMenu(MENU_PATH);
    _container->addMenuItem(PluginType::INPUT_PLUGIN, MENU_PATH, RENDER_CONTROLLERS,
        [this] (bool clicked) { this->setRenderControllers(clicked); },
        true, true);

    if (!_hmd) {
        _hmd = acquireOpenVrSystem();
    }
    Q_ASSERT(_hmd);

    // OpenVR provides 3d mesh representations of the controllers
    // Disabled controller rendering code
    /*
    auto renderModels = vr::VRRenderModels();

    vr::RenderModel_t model;
    if (!_hmd->LoadRenderModel(CONTROLLER_MODEL_STRING, &model)) {
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

        gpu::Element formatGPU = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        gpu::Element formatMip = gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA);
        _texture = gpu::TexturePointer(
            gpu::Texture::create2D(formatGPU, model.diffuseTexture.unWidth, model.diffuseTexture.unHeight,
            gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
        _texture->assignStoredMip(0, formatMip, model.diffuseTexture.unWidth * model.diffuseTexture.unHeight * 4 * sizeof(uint8_t), model.diffuseTexture.rubTextureMapData);
        _texture->autoGenerateMips(-1);

        _modelLoaded = true;
        _renderControllers = true;
    }
    */

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(_inputDevice);
    _registeredWithInputMapper = true;
}

void ViveControllerManager::deactivate() {
    InputPlugin::deactivate();

    _container->removeMenuItem(MENU_NAME, RENDER_CONTROLLERS);
    _container->removeMenu(MENU_PATH);

    if (_hmd) {
        releaseOpenVrSystem();
        _hmd = nullptr;
    }

    _inputDevice->_poseStateMap.clear();

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->removeDevice(_inputDevice->_deviceID);
    _registeredWithInputMapper = false;
}

void ViveControllerManager::updateRendering(RenderArgs* args, render::ScenePointer scene, render::PendingChanges pendingChanges) {
    PerformanceTimer perfTimer("ViveControllerManager::updateRendering");

    /*
    if (_modelLoaded) {
        //auto controllerPayload = new render::Payload<ViveControllerManager>(this);
        //auto controllerPayloadPointer = ViveControllerManager::PayloadPointer(controllerPayload);
        //if (_leftHandRenderID == 0) {
        //    _leftHandRenderID = scene->allocateID();
        //    pendingChanges.resetItem(_leftHandRenderID, controllerPayloadPointer);
        //}
        //pendingChanges.updateItem(_leftHandRenderID, );


        controller::Pose leftHand = _inputDevice->_poseStateMap[controller::StandardPoseChannel::LEFT_HAND];
        controller::Pose rightHand = _inputDevice->_poseStateMap[controller::StandardPoseChannel::RIGHT_HAND];

        gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
            auto geometryCache = DependencyManager::get<GeometryCache>();
            geometryCache->useSimpleDrawPipeline(batch);
            DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch, true);

            auto mesh = _modelGeometry.getMesh();
            batch.setInputFormat(mesh->getVertexFormat());
            //batch._glBindTexture(GL_TEXTURE_2D, _uexture);

            if (leftHand.isValid()) {
                renderHand(leftHand, batch, 1);
            }
            if (rightHand.isValid()) {
                renderHand(rightHand, batch, -1);
            }
        });
    }
    */
}

void ViveControllerManager::renderHand(const controller::Pose& pose, gpu::Batch& batch, int sign) {
    /*
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    Transform transform(userInputMapper->getSensorToWorldMat());
    transform.postTranslate(pose.getTranslation() + pose.getRotation() * glm::vec3(0, 0, CONTROLLER_LENGTH_OFFSET));

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
    */
}


void ViveControllerManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) {
    _inputDevice->update(deltaTime, inputCalibrationData, jointsCaptured);
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();

    if (_inputDevice->_trackedControllers == 0 && _registeredWithInputMapper) {
        userInputMapper->removeDevice(_inputDevice->_deviceID);
        _registeredWithInputMapper = false;
        _inputDevice->_poseStateMap.clear();
    }

    if (!_registeredWithInputMapper && _inputDevice->_trackedControllers > 0) {
        userInputMapper->registerDevice(_inputDevice);
        _registeredWithInputMapper = true;
        UserActivityLogger::getInstance().connectedDevice("spatial_controller", "steamVR");
    }
}

void ViveControllerManager::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) {
    _poseStateMap.clear();

    _buttonPressedMap.clear();

    PerformanceTimer perfTimer("ViveControllerManager::update");

    int numTrackedControllers = 0;

    for (vr::TrackedDeviceIndex_t device = vr::k_unTrackedDeviceIndex_Hmd + 1;
         device < vr::k_unMaxTrackedDeviceCount && numTrackedControllers < 2; ++device) {

        if (!_hmd->IsTrackedDeviceConnected(device)) {
            continue;
        }

        if (_hmd->GetTrackedDeviceClass(device) != vr::TrackedDeviceClass_Controller) {
            continue;
        }

        if (!_trackedDevicePose[device].bPoseIsValid) {
            continue;
        }

        numTrackedControllers++;
        bool left = numTrackedControllers == 2;

        const mat4& mat = _trackedDevicePoseMat4[device];

        if (!jointsCaptured) {
            handlePoseEvent(inputCalibrationData, mat, numTrackedControllers - 1);
        }

        // handle inputs
        vr::VRControllerState_t controllerState = vr::VRControllerState_t();
        if (_hmd->GetControllerState(device, &controllerState)) {
            //qDebug() << (numTrackedControllers == 1 ? "Left: " : "Right: ");
            //qDebug() << "Trackpad: " << controllerState.rAxis[0].x << " " << controllerState.rAxis[0].y;
            //qDebug() << "Trigger: " << controllerState.rAxis[1].x << " " << controllerState.rAxis[1].y;
            for (uint32_t i = 0; i < vr::k_EButton_Max; ++i) {
                auto mask = vr::ButtonMaskFromId((vr::EVRButtonId)i);
                bool pressed = 0 != (controllerState.ulButtonPressed & mask);
                handleButtonEvent(i, pressed, left);
            }
            for (uint32_t i = 0; i < vr::k_unControllerStateAxisCount; i++) {
                handleAxisEvent(i, controllerState.rAxis[i].x, controllerState.rAxis[i].y, left);
            }
        }
    }

    _trackedControllers = numTrackedControllers;
}

void ViveControllerManager::InputDevice::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

// These functions do translation from the Steam IDs to the standard controller IDs
void ViveControllerManager::InputDevice::handleAxisEvent(uint32_t axis, float x, float y, bool left) {
    //FIX ME? It enters here every frame: probably we want to enter only if an event occurs
    axis += vr::k_EButton_Axis0;
    using namespace controller;
    if (axis == vr::k_EButton_SteamVR_Touchpad) {
        _axisStateMap[left ? LX : RX] = x;
        _axisStateMap[left ? LY : RY] = y;
    } else if (axis == vr::k_EButton_SteamVR_Trigger) {
        _axisStateMap[left ? LT : RT] = x;
    }
}

// These functions do translation from the Steam IDs to the standard controller IDs
void ViveControllerManager::InputDevice::handleButtonEvent(uint32_t button, bool pressed, bool left) {
    if (!pressed) {
        return;
    }

    if (button == vr::k_EButton_ApplicationMenu) {
        _buttonPressedMap.insert(left ? controller::LEFT_PRIMARY_THUMB : controller::RIGHT_PRIMARY_THUMB);
    } else if (button == vr::k_EButton_Grip) {
        // Tony says these are harder to reach, so make them the meta buttons
        _buttonPressedMap.insert(left ? controller::LB : controller::RB);
    } else if (button == vr::k_EButton_SteamVR_Trigger) {
        _buttonPressedMap.insert(left ? controller::LT : controller::RT);
    } else if (button == vr::k_EButton_SteamVR_Touchpad) {
        _buttonPressedMap.insert(left ? controller::LS : controller::RS);
    } else if (button == vr::k_EButton_System) {
        //FIX ME: not able to ovrewrite the behaviour of this button
        _buttonPressedMap.insert(left ? controller::LEFT_SECONDARY_THUMB : controller::RIGHT_SECONDARY_THUMB);
    }
}

void ViveControllerManager::InputDevice::handlePoseEvent(const controller::InputCalibrationData& inputCalibrationData, const mat4& mat, bool left) {
    // When the sensor-to-world rotation is identity the coordinate axes look like this:
    //
    //                       user
    //                      forward
    //                        -z
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

    static const glm::quat yFlip = glm::angleAxis(PI, Vectors::UNIT_Y);
    static const glm::quat quarterX = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_X);
    static const glm::quat viveToHand = yFlip * quarterX;

    static const glm::quat leftQuaterZ = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat rightQuaterZ = glm::angleAxis(PI_OVER_TWO, Vectors::UNIT_Z);
    static const glm::quat eighthX = glm::angleAxis(PI / 4.0f, Vectors::UNIT_X);

    static const glm::quat leftRotationOffset = glm::inverse(leftQuaterZ * eighthX) * viveToHand;
    static const glm::quat rightRotationOffset = glm::inverse(rightQuaterZ * eighthX) * viveToHand;

    static const glm::vec3 leftTranslationOffset = glm::vec3(-1.0f, 1.0f, 1.0f) * CONTROLLER_OFFSET;
    static const glm::vec3 rightTranslationOffset = CONTROLLER_OFFSET;

    glm::vec3 position = extractTranslation(mat);
    glm::quat rotation = glm::quat_cast(mat);

    position += rotation * (left ? leftTranslationOffset : rightTranslationOffset);
    rotation = rotation * (left ? leftRotationOffset : rightRotationOffset);

    // transform into avatar frame
    glm::mat4 controllerToAvatar = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    auto avatarPose = controller::Pose(position, rotation).transform(controllerToAvatar);
    _poseStateMap[left ? controller::LEFT_HAND : controller::RIGHT_HAND] = avatarPose;
}

controller::Input::NamedVector ViveControllerManager::InputDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs{
        // Trackpad analogs
        makePair(LX, "LX"),
        makePair(LY, "LY"),
        makePair(RX, "RX"),
        makePair(RY, "RY"),
        // trigger analogs
        makePair(LT, "LT"),
        makePair(RT, "RT"),

        makePair(LB, "LB"),
        makePair(RB, "RB"),

        makePair(LS, "LS"),
        makePair(RS, "RS"),
        makePair(LEFT_HAND, "LeftHand"),
        makePair(RIGHT_HAND, "RightHand"),

        makePair(LEFT_PRIMARY_THUMB, "LeftPrimaryThumb"),
        makePair(LEFT_SECONDARY_THUMB, "LeftSecondaryThumb"),
        makePair(RIGHT_PRIMARY_THUMB, "RightPrimaryThumb"),
        makePair(RIGHT_SECONDARY_THUMB, "RightSecondaryThumb"),
    };

    //availableInputs.append(Input::NamedPair(makeInput(BUTTON_A, 0), "Left Button A"));
    //availableInputs.append(Input::NamedPair(makeInput(GRIP_BUTTON, 0), "Left Grip Button"));
    //availableInputs.append(Input::NamedPair(makeInput(TRACKPAD_BUTTON, 0), "Left Trackpad Button"));
    //availableInputs.append(Input::NamedPair(makeInput(TRIGGER_BUTTON, 0), "Left Trigger Button"));
    //availableInputs.append(Input::NamedPair(makeInput(BACK_TRIGGER, 0), "Left Back Trigger"));
    //availableInputs.append(Input::NamedPair(makeInput(RIGHT_HAND), "Right Hand"));
    //availableInputs.append(Input::NamedPair(makeInput(BUTTON_A, 1), "Right Button A"));
    //availableInputs.append(Input::NamedPair(makeInput(GRIP_BUTTON, 1), "Right Grip Button"));
    //availableInputs.append(Input::NamedPair(makeInput(TRACKPAD_BUTTON, 1), "Right Trackpad Button"));
    //availableInputs.append(Input::NamedPair(makeInput(TRIGGER_BUTTON, 1), "Right Trigger Button"));
    //availableInputs.append(Input::NamedPair(makeInput(BACK_TRIGGER, 1), "Right Back Trigger"));

    return availableInputs;
}

QString ViveControllerManager::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/vive.json";
    return MAPPING_JSON;
}

//void ViveControllerManager::assignDefaultInputMapping(UserInputMapper& mapper) {
//    const float JOYSTICK_MOVE_SPEED = 1.0f;
//
//    // Left Trackpad: Movement, strafing
//    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(AXIS_Y_POS, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(AXIS_Y_NEG, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(AXIS_X_POS, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(AXIS_X_NEG, 0), makeInput(TRACKPAD_BUTTON, 0), JOYSTICK_MOVE_SPEED);
//
//    // Right Trackpad: Vertical movement, zooming
//    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(AXIS_Y_POS, 1), makeInput(TRACKPAD_BUTTON, 1), JOYSTICK_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(AXIS_Y_NEG, 1), makeInput(TRACKPAD_BUTTON, 1), JOYSTICK_MOVE_SPEED);
//
//    // Buttons
//    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(BUTTON_A, 0));
//    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(BUTTON_A, 1));
//
//    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(GRIP_BUTTON, 0));
//    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(GRIP_BUTTON, 1));
//
//    mapper.addInputChannel(UserInputMapper::LEFT_HAND_CLICK, makeInput(BACK_TRIGGER, 0));
//    mapper.addInputChannel(UserInputMapper::RIGHT_HAND_CLICK, makeInput(BACK_TRIGGER, 1));
//
//    // Hands
//    mapper.addInputChannel(UserInputMapper::LEFT_HAND, makeInput(LEFT_HAND));
//    mapper.addInputChannel(UserInputMapper::RIGHT_HAND, makeInput(RIGHT_HAND));
//}
