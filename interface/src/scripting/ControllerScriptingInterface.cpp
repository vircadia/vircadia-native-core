//
//  ControllerScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 12/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ControllerScriptingInterface.h"

#include <avatar/AvatarManager.h>
#include <avatar/MyAvatar.h>
#include <HandData.h>
#include <HFBackEvent.h>
#include <plugins/PluginManager.h>

#include "Application.h"
#include "devices/MotionTracker.h"

// TODO: this needs to be removed, as well as any related controller-specific information
#include <input-plugins/SixenseManager.h>

void ControllerScriptingInterface::handleMetaEvent(HFMetaEvent* event) {
    if (event->type() == HFActionEvent::startType()) {
        emit actionStartEvent(static_cast<HFActionEvent&>(*event));
    } else if (event->type() == HFActionEvent::endType()) {
        emit actionEndEvent(static_cast<HFActionEvent&>(*event));
    } else if (event->type() == HFBackEvent::startType()) {
        emit backStartEvent();
    } else if (event->type() == HFBackEvent::endType()) {
        emit backEndEvent();
    }
}

const PalmData* ControllerScriptingInterface::getPrimaryPalm() const {
    int leftPalmIndex, rightPalmIndex;

    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();
    handData->getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);
    
    if (rightPalmIndex != -1) {
        return &handData->getPalms()[rightPalmIndex];
    }

    return NULL;
}

int ControllerScriptingInterface::getNumberOfActivePalms() const {
    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();
    int numberOfPalms = handData->getNumPalms();
    int numberOfActivePalms = 0;
    for (int i = 0; i < numberOfPalms; i++) {
        if (getPalm(i)->isActive()) {
            numberOfActivePalms++;
        }
    }
    return numberOfActivePalms;
}

const PalmData* ControllerScriptingInterface::getPalm(int palmIndex) const {
    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();
    return &handData->getPalms()[palmIndex];
}

const PalmData* ControllerScriptingInterface::getActivePalm(int palmIndex) const {
    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();
    int numberOfPalms = handData->getNumPalms();
    int numberOfActivePalms = 0;
    for (int i = 0; i < numberOfPalms; i++) {
        if (getPalm(i)->isActive()) {
            if (numberOfActivePalms == palmIndex) {
                return &handData->getPalms()[i];
            }
            numberOfActivePalms++;
        }
    }
    return NULL;
}

bool ControllerScriptingInterface::isKeyCaptured(QKeyEvent* event) const {
    return isKeyCaptured(KeyEvent(*event));
}

bool ControllerScriptingInterface::isKeyCaptured(const KeyEvent& event) const {
    // if we've captured some combination of this key it will be in the map
    return _capturedKeys.contains(event.key, event);
}

void ControllerScriptingInterface::captureKeyEvents(const KeyEvent& event) {
    // if it's valid
    if (event.isValid) {
        // and not already captured
        if (!isKeyCaptured(event)) {
            // then add this KeyEvent record to the captured combos for this key
            _capturedKeys.insert(event.key, event);
        }
    }
}

void ControllerScriptingInterface::releaseKeyEvents(const KeyEvent& event) {
    if (event.isValid) {
        // and not already captured
        if (isKeyCaptured(event)) {
            _capturedKeys.remove(event.key, event);
        }
    }
}

bool ControllerScriptingInterface::isJoystickCaptured(int joystickIndex) const {
    return _capturedJoysticks.contains(joystickIndex);
}

void ControllerScriptingInterface::captureJoystick(int joystickIndex) {
    if (!isJoystickCaptured(joystickIndex)) {
        _capturedJoysticks.insert(joystickIndex);
    }
}

void ControllerScriptingInterface::releaseJoystick(int joystickIndex) {
    if (isJoystickCaptured(joystickIndex)) {
        _capturedJoysticks.remove(joystickIndex);
    }
}

glm::vec2 ControllerScriptingInterface::getViewportDimensions() const {
    return qApp->getUiSize();
}

controller::InputController::Pointer ControllerScriptingInterface::createInputController(const QString& deviceName, const QString& tracker) {
    // This is where we retreive the Device Tracker category and then the sub tracker within it
    auto icIt = _inputControllers.find(0);
    if (icIt != _inputControllers.end()) {
        return (*icIt).second;
    } 


    // Look for device
    DeviceTracker::ID deviceID = DeviceTracker::getDeviceID(deviceName.toStdString());
    if (deviceID < 0) {
        deviceID = 0;
    }
    // TODO in this current implementation, we just pick the device assuming there is one (normally the Leapmotion)
    // in the near future we need to change that to a real mapping between the devices and the deviceName
    // ALso we need to expand the spec so we can fall back on  the "default" controller per categories

    if (deviceID >= 0) {
        // TODO here again the assumption it's the LeapMotion and so it's a MOtionTracker, this would need to be changed to support different types of devices
        MotionTracker* motionTracker = dynamic_cast< MotionTracker* > (DeviceTracker::getDevice(deviceID));
        if (motionTracker) {
            MotionTracker::Index trackerID = motionTracker->findJointIndex(tracker.toStdString());
            if (trackerID >= 0) {
                controller::InputController::Pointer inputController = std::make_shared<InputController>(deviceID, trackerID, this);
                controller::InputController::Key key = inputController->getKey();
                _inputControllers.insert(InputControllerMap::value_type(key, inputController));
                return inputController;
            }
        }
    }

    return controller::InputController::Pointer();
}

void ControllerScriptingInterface::releaseInputController(controller::InputController::Pointer input) {
    _inputControllers.erase(input->getKey());
}

void ControllerScriptingInterface::update() {
    static float last = secTimestampNow();
    float now = secTimestampNow();
    float delta = now - last;
    last = now;

    DependencyManager::get<UserInputMapper>()->update(delta);

    bool jointsCaptured = false;
    for (auto inputPlugin : PluginManager::getInstance()->getInputPlugins()) {
        if (inputPlugin->isActive()) {
            inputPlugin->pluginUpdate(delta, jointsCaptured);
            if (inputPlugin->isJointController()) {
                jointsCaptured = true;
            }
        }
    }

    for (auto entry : _inputControllers) {
        entry.second->update();
    }

    controller::ScriptingInterface::update();
}

InputController::InputController(int deviceTrackerId, int subTrackerId, QObject* parent) :
    _deviceTrackerId(deviceTrackerId),
    _subTrackerId(subTrackerId),
    _isActive(false)
{
}

void InputController::update() {
    _isActive = false;

    // TODO for now the InputController is only supporting a JointTracker from a MotionTracker
    MotionTracker* motionTracker = dynamic_cast< MotionTracker*> (DeviceTracker::getDevice(_deviceTrackerId));
    if (motionTracker) {
        if ((int)_subTrackerId < motionTracker->numJointTrackers()) {
            const MotionTracker::JointTracker* joint = motionTracker->getJointTracker(_subTrackerId);

            if (joint->isActive()) {
                joint->getAbsFrame().getTranslation(_eventCache.absTranslation);
                joint->getAbsFrame().getRotation(_eventCache.absRotation);
                joint->getLocFrame().getTranslation(_eventCache.locTranslation);
                joint->getLocFrame().getRotation(_eventCache.locRotation);

                _isActive = true;
                //emit spatialEvent(_eventCache);
            }
        }
    }
}

const unsigned int INPUTCONTROLLER_KEY_DEVICE_OFFSET = 16;
const unsigned int INPUTCONTROLLER_KEY_DEVICE_MASK = 16;

InputController::Key InputController::getKey() const {
    return (((_deviceTrackerId & INPUTCONTROLLER_KEY_DEVICE_MASK) << INPUTCONTROLLER_KEY_DEVICE_OFFSET) | _subTrackerId);
}


void ControllerScriptingInterface::emitKeyPressEvent(QKeyEvent* event) { emit keyPressEvent(KeyEvent(*event)); }
void ControllerScriptingInterface::emitKeyReleaseEvent(QKeyEvent* event) { emit keyReleaseEvent(KeyEvent(*event)); }

void ControllerScriptingInterface::emitMouseMoveEvent(QMouseEvent* event, unsigned int deviceID) { emit mouseMoveEvent(MouseEvent(*event, deviceID)); }
void ControllerScriptingInterface::emitMousePressEvent(QMouseEvent* event, unsigned int deviceID) { emit mousePressEvent(MouseEvent(*event, deviceID)); }
void ControllerScriptingInterface::emitMouseDoublePressEvent(QMouseEvent* event, unsigned int deviceID) { emit mouseDoublePressEvent(MouseEvent(*event, deviceID)); }
void ControllerScriptingInterface::emitMouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) { emit mouseReleaseEvent(MouseEvent(*event, deviceID)); }

void ControllerScriptingInterface::emitTouchBeginEvent(const TouchEvent& event) { emit touchBeginEvent(event); }
void ControllerScriptingInterface::emitTouchEndEvent(const TouchEvent& event) { emit touchEndEvent(event); }
void ControllerScriptingInterface::emitTouchUpdateEvent(const TouchEvent& event) { emit touchUpdateEvent(event); }

void ControllerScriptingInterface::emitWheelEvent(QWheelEvent* event) { emit wheelEvent(*event); }

