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
#include <display-plugins/openvr/OpenVrHelpers.h>
#include <NumericalConstants.h>
#include <plugins/PluginContainer.h>
#include <UserActivityLogger.h>

#include <controllers/UserInputMapper.h>

#include <controllers/StandardControls.h>

#ifdef Q_OS_WIN
extern vr::IVRSystem* _hmd;
extern int hmdRefCount;
extern vr::TrackedDevicePose_t _trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
extern mat4 _trackedDevicePoseMat4[vr::k_unMaxTrackedDeviceCount];
#endif


const float CONTROLLER_LENGTH_OFFSET = 0.0762f;  // three inches
const QString CONTROLLER_MODEL_STRING = "vr_controller_05_wireless_b";

const QString ViveControllerManager::NAME = "OpenVR";

const QString MENU_PARENT = "Avatar";
const QString MENU_NAME = "Vive Controllers";
const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
const QString RENDER_CONTROLLERS = "Render Hand Controllers";

static std::shared_ptr<ViveControllerManager> instance;

ViveControllerManager::ViveControllerManager() :
        InputDevice("Vive"),
    _trackedControllers(0),
    _modelLoaded(false),
    _leftHandRenderID(0),
    _rightHandRenderID(0),
    _renderControllers(false)
{
    instance = std::shared_ptr<ViveControllerManager>(this);
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
    InputPlugin::activate();
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

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(instance);
    _registeredWithInputMapper = true;
}

void ViveControllerManager::deactivate() {
    InputPlugin::deactivate();

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

    // unregister with UserInputMapper
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->removeDevice(_deviceID);
    _registeredWithInputMapper = false;
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


        controller::Pose leftHand = _poseStateMap[controller::StandardPoseChannel::LEFT_HAND];
        controller::Pose rightHand = _poseStateMap[controller::StandardPoseChannel::RIGHT_HAND];

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
}

void ViveControllerManager::renderHand(const controller::Pose& pose, gpu::Batch& batch, int sign) {
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
        bool left = numTrackedControllers == 1;
            
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
        
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        
    if (numTrackedControllers == 0) {
        if (_registeredWithInputMapper) {
            userInputMapper->removeDevice(_deviceID);
            _registeredWithInputMapper = false;
            _poseStateMap.clear();
        }
    }
        
    if (_trackedControllers == 0 && numTrackedControllers > 0) {
        userInputMapper->registerDevice(instance);
        _registeredWithInputMapper = true;
        UserActivityLogger::getInstance().connectedDevice("spatial_controller", "steamVR");
    }
        
    _trackedControllers = numTrackedControllers;
#endif
}

void ViveControllerManager::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

// These functions do translation from the Steam IDs to the standard controller IDs
void ViveControllerManager::handleAxisEvent(uint32_t axis, float x, float y, bool left) {
#ifdef Q_OS_WIN
    using namespace controller;
    if (axis == vr::k_EButton_SteamVR_Touchpad) {
        _axisStateMap[left ? LX : RX] = x;
        _axisStateMap[left ? LY : RY] = y;
    } else if (axis == vr::k_EButton_SteamVR_Trigger) {
        _axisStateMap[left ? LT : RT] = x;
    }
#endif
}

// These functions do translation from the Steam IDs to the standard controller IDs
void ViveControllerManager::handleButtonEvent(uint32_t button, bool pressed, bool left) {
#ifdef Q_OS_WIN
    if (!pressed) {
        return;
    }

    if (button == vr::k_EButton_ApplicationMenu) {
        // FIXME?
        _buttonPressedMap.insert(left ? controller::B : controller::A);
    } else if (button == vr::k_EButton_Grip) {
        // Tony says these are harder to reach, so make them the meta buttons
        _buttonPressedMap.insert(left ? controller::BACK : controller::START);
    } else if (button == vr::k_EButton_SteamVR_Trigger) {
        _buttonPressedMap.insert(left ? controller::LB : controller::RB);
    } else if (button == vr::k_EButton_SteamVR_Touchpad) {
        _buttonPressedMap.insert(left ? controller::LS : controller::RS);
    }
#endif
}

void ViveControllerManager::handlePoseEvent(const mat4& mat, bool left) {
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
    float sign = left ? -1.0f : 1.0f;
    const glm::quat signedQuaterZ = glm::angleAxis(sign * PI / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f)); 
    const glm::quat eighthX = glm::angleAxis(PI / 4.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat offset = glm::inverse(signedQuaterZ * eighthX);
    rotation = rotation * offset * yFlip * quarterX;

    position += rotation * glm::vec3(0, 0, -CONTROLLER_LENGTH_OFFSET);

    _poseStateMap[left ? controller::LEFT_HAND : controller::RIGHT_HAND] = controller::Pose(position, rotation);
}

controller::Input::NamedVector ViveControllerManager::getAvailableInputs() const {
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

        makePair(A, "A"),
        makePair(B, "B"),
        makePair(BACK, "Back"),
        makePair(START, "Start"),
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

QString ViveControllerManager::getDefaultMappingConfig() const {
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
