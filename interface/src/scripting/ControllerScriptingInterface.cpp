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
#include <HFBackEvent.h>
#include <plugins/PluginManager.h>

#include "Application.h"
#include "devices/MotionTracker.h"

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

QVariant ControllerScriptingInterface::getRecommendedOverlayRect() const {
    auto rect = qApp->getRecommendedOverlayRect();
    return qRectToVariant(rect);
}

controller::InputController* ControllerScriptingInterface::createInputController(const QString& deviceName, const QString& tracker) {
    // This is where we retrieve the Device Tracker category and then the sub tracker within it
    auto icIt = _inputControllers.find(0);
    if (icIt != _inputControllers.end()) {
        return (*icIt).second.get();
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
                return inputController.get();
            }
        }
    }

    return nullptr;
}

void ControllerScriptingInterface::releaseInputController(controller::InputController* input) {
    _inputControllers.erase(input->getKey());
}

void ControllerScriptingInterface::updateInputControllers() {
    for (auto it = _inputControllers.begin(); it != _inputControllers.end(); it++) {
        (*it).second->update();
    }
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

void ControllerScriptingInterface::emitMouseMoveEvent(QMouseEvent* event) { emit mouseMoveEvent(MouseEvent(*event)); }
void ControllerScriptingInterface::emitMousePressEvent(QMouseEvent* event) { emit mousePressEvent(MouseEvent(*event)); }
void ControllerScriptingInterface::emitMouseDoublePressEvent(QMouseEvent* event) { emit mouseDoublePressEvent(MouseEvent(*event)); }
void ControllerScriptingInterface::emitMouseReleaseEvent(QMouseEvent* event) { emit mouseReleaseEvent(MouseEvent(*event)); }

void ControllerScriptingInterface::emitTouchBeginEvent(const TouchEvent& event) { emit touchBeginEvent(event); }
void ControllerScriptingInterface::emitTouchEndEvent(const TouchEvent& event) { emit touchEndEvent(event); }
void ControllerScriptingInterface::emitTouchUpdateEvent(const TouchEvent& event) { emit touchUpdateEvent(event); }

void ControllerScriptingInterface::emitWheelEvent(QWheelEvent* event) { emit wheelEvent(*event); }

