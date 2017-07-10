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

bool ControllerScriptingInterface::areEntityClicksCaptured() const {
    return _captureEntityClicks;
}

void ControllerScriptingInterface::captureEntityClickEvents() {
    _captureEntityClicks = true;
}

void ControllerScriptingInterface::releaseEntityClickEvents() {
    _captureEntityClicks = false;
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

