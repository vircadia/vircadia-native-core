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
#include <ui-plugins/PluginContainer.h>
#include <UserActivityLogger.h>
#include <OffscreenUi.h>


#include <controllers/UserInputMapper.h>

#include <controllers/StandardControls.h>

#include "OpenVrHelpers.h"

extern PoseData _nextSimPoseData;

vr::IVRSystem* acquireOpenVrSystem();
void releaseOpenVrSystem();


static const char* CONTROLLER_MODEL_STRING = "vr_controller_05_wireless_b";

static const QString MENU_PARENT = "Avatar";
static const QString MENU_NAME = "Vive Controllers";
static const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
static const QString RENDER_CONTROLLERS = "Render Hand Controllers";

const QString ViveControllerManager::NAME = "OpenVR";

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

    // OpenVR provides 3d mesh representations of the controllers
    // Disabled controller rendering code
    /*
    auto renderModels = vr::VRRenderModels();

    vr::RenderModel_t model;
    if (!_system->LoadRenderModel(CONTROLLER_MODEL_STRING, &model)) {
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
        releaseOpenVrSystem();
        _system = nullptr;
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


void ViveControllerManager::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
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

void ViveControllerManager::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _poseStateMap.clear();
    _buttonPressedMap.clear();

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
        if (_system->GetControllerState(deviceIndex, &controllerState)) {
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
