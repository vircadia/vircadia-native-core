//
//  KeyboardMouseDevice.cpp
//  input-plugins/src/input-plugins
//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "KeyboardMouseDevice.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QTouchEvent>

#include <controllers/UserInputMapper.h>
#include <PathUtils.h>

const QString KeyboardMouseDevice::NAME = "Keyboard/Mouse";

void KeyboardMouseDevice::update(float deltaTime, bool jointsCaptured) {
    _axisStateMap.clear();

    // For touch event, we need to check that the last event is not too long ago
    // Maybe it's a Qt issue, but the touch event sequence (begin, update, end) is not always called properly
    // The following is a workaround to detect that the touch sequence is over in case we didn;t see the end event
    if (_isTouching) {
        const auto TOUCH_EVENT_MAXIMUM_WAIT = 100; //ms
        auto currentTime = _clock.now();
        auto sinceLastTouch = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - _lastTouchTime);
        if (sinceLastTouch.count() > TOUCH_EVENT_MAXIMUM_WAIT) {
            _isTouching = false;
        }
    }
}

void KeyboardMouseDevice::focusOutEvent() {
    _buttonPressedMap.clear();
};

void KeyboardMouseDevice::keyPressEvent(QKeyEvent* event) {
    auto input = makeInput((Qt::Key) event->key());
    auto result = _buttonPressedMap.insert(input.getChannel());
    if (!result.second) {
        // key pressed again ? without catching the release event ?
    }
}

void KeyboardMouseDevice::keyReleaseEvent(QKeyEvent* event) {
    auto input = makeInput((Qt::Key) event->key());
    _buttonPressedMap.erase(input.getChannel());
}

void KeyboardMouseDevice::mousePressEvent(QMouseEvent* event, unsigned int deviceID) {
    auto input = makeInput((Qt::MouseButton) event->button());
    auto result = _buttonPressedMap.insert(input.getChannel());
    if (!result.second) {
        // key pressed again ? without catching the release event ?
    }
    _lastCursor = event->pos();
}

void KeyboardMouseDevice::mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) {
    auto input = makeInput((Qt::MouseButton) event->button());
    _buttonPressedMap.erase(input.getChannel());
}

void KeyboardMouseDevice::mouseMoveEvent(QMouseEvent* event, unsigned int deviceID) {
    QPoint currentPos = event->pos();
    QPoint currentMove = currentPos - _lastCursor;

    _axisStateMap[makeInput(MOUSE_AXIS_X_POS).getChannel()] = (currentMove.x() > 0 ? currentMove.x() : 0.0f);
    _axisStateMap[makeInput(MOUSE_AXIS_X_NEG).getChannel()] = (currentMove.x() < 0 ? -currentMove.x() : 0.0f);
     // Y mouse is inverted positive is pointing up the screen
    _axisStateMap[makeInput(MOUSE_AXIS_Y_POS).getChannel()] = (currentMove.y() < 0 ? -currentMove.y() : 0.0f);
    _axisStateMap[makeInput(MOUSE_AXIS_Y_NEG).getChannel()] = (currentMove.y() > 0 ? currentMove.y() : 0.0f);

    _lastCursor = currentPos;
}

void KeyboardMouseDevice::wheelEvent(QWheelEvent* event) {
    auto currentMove = event->angleDelta() / 120.0f;

    _axisStateMap[makeInput(MOUSE_AXIS_WHEEL_X_POS).getChannel()] = (currentMove.x() > 0 ? currentMove.x() : 0.0f);
    _axisStateMap[makeInput(MOUSE_AXIS_WHEEL_X_NEG).getChannel()] = (currentMove.x() < 0 ? -currentMove.x() : 0.0f);
    _axisStateMap[makeInput(MOUSE_AXIS_WHEEL_Y_POS).getChannel()] = (currentMove.y() > 0 ? currentMove.y() : 0.0f);
    _axisStateMap[makeInput(MOUSE_AXIS_WHEEL_Y_NEG).getChannel()] = (currentMove.y() < 0 ? -currentMove.y() : 0.0f);
}

glm::vec2 evalAverageTouchPoints(const QList<QTouchEvent::TouchPoint>& points) {
    glm::vec2 averagePoint(0.0f);
    if (points.count() > 0) {
        for (auto& point : points) {
            averagePoint += glm::vec2(point.pos().x(), point.pos().y()); 
        }
        averagePoint /= (float)(points.count());
    }
    return averagePoint;
}

void KeyboardMouseDevice::touchBeginEvent(const QTouchEvent* event) {
    _isTouching = event->touchPointStates().testFlag(Qt::TouchPointPressed);
    _lastTouch = evalAverageTouchPoints(event->touchPoints());
    _lastTouchTime = _clock.now();
}

void KeyboardMouseDevice::touchEndEvent(const QTouchEvent* event) {
    _isTouching = false;
    _lastTouch = evalAverageTouchPoints(event->touchPoints());
    _lastTouchTime = _clock.now();
}

void KeyboardMouseDevice::touchUpdateEvent(const QTouchEvent* event) {
    auto currentPos = evalAverageTouchPoints(event->touchPoints());
    _lastTouchTime = _clock.now();
    
    if (!_isTouching) {
        _isTouching = event->touchPointStates().testFlag(Qt::TouchPointPressed);
    } else {
        auto currentMove = currentPos - _lastTouch;
    
        _axisStateMap[makeInput(TOUCH_AXIS_X_POS).getChannel()] = (currentMove.x > 0 ? currentMove.x : 0.0f);
        _axisStateMap[makeInput(TOUCH_AXIS_X_NEG).getChannel()] = (currentMove.x < 0 ? -currentMove.x : 0.0f);
        // Y mouse is inverted positive is pointing up the screen
        _axisStateMap[makeInput(TOUCH_AXIS_Y_POS).getChannel()] = (currentMove.y < 0 ? -currentMove.y : 0.0f);
        _axisStateMap[makeInput(TOUCH_AXIS_Y_NEG).getChannel()] = (currentMove.y > 0 ? currentMove.y : 0.0f);
    }

    _lastTouch = currentPos;
}

controller::Input KeyboardMouseDevice::makeInput(Qt::Key code) {
    auto shortCode = (uint16_t)(code & KEYBOARD_MASK);
    if (shortCode != code) {
       shortCode |= 0x0800; // add this bit instead of the way Qt::Key add a bit on the 3rd byte for some keys
    }
    return controller::Input(_deviceID, shortCode, controller::ChannelType::BUTTON);
}

controller::Input KeyboardMouseDevice::makeInput(Qt::MouseButton code) {
    switch (code) {
        case Qt::LeftButton:
            return controller::Input(_deviceID, MOUSE_BUTTON_LEFT, controller::ChannelType::BUTTON);
        case Qt::RightButton:
            return controller::Input(_deviceID, MOUSE_BUTTON_RIGHT, controller::ChannelType::BUTTON);
        case Qt::MiddleButton:
            return controller::Input(_deviceID, MOUSE_BUTTON_MIDDLE, controller::ChannelType::BUTTON);
        default:
            return controller::Input();
    };
}

controller::Input KeyboardMouseDevice::makeInput(KeyboardMouseDevice::MouseAxisChannel axis) {
    return controller::Input(_deviceID, axis, controller::ChannelType::AXIS);
}

controller::Input KeyboardMouseDevice::makeInput(KeyboardMouseDevice::TouchAxisChannel axis) {
    return controller::Input(_deviceID, axis, controller::ChannelType::AXIS);
}

controller::Input KeyboardMouseDevice::makeInput(KeyboardMouseDevice::TouchButtonChannel button) {
    return controller::Input(_deviceID, button, controller::ChannelType::BUTTON);
}

void KeyboardMouseDevice::buildDeviceProxy(controller::DeviceProxy::Pointer proxy) {
    using namespace controller;
    proxy->getButton = [this] (const controller::Input& input, int timestamp) -> bool { return this->getButton(input.getChannel()); };
    proxy->getAxis = [this] (const controller::Input& input, int timestamp) -> float { return this->getAxis(input.getChannel()); };
    proxy->getAvailabeInputs = [this] () -> QVector<Input::NamedPair> {
        QVector<Input::NamedPair> availableInputs;
        for (int i = (int) Qt::Key_0; i <= (int) Qt::Key_9; i++) {
            availableInputs.append(Input::NamedPair(makeInput(Qt::Key(i)), QKeySequence(Qt::Key(i)).toString()));
        }
        for (int i = (int) Qt::Key_A; i <= (int) Qt::Key_Z; i++) {
            availableInputs.append(Input::NamedPair(makeInput(Qt::Key(i)), QKeySequence(Qt::Key(i)).toString()));
        }
        for (int i = (int) Qt::Key_Left; i <= (int) Qt::Key_Down; i++) {
            availableInputs.append(Input::NamedPair(makeInput(Qt::Key(i)), QKeySequence(Qt::Key(i)).toString()));
        }
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_Space), QKeySequence(Qt::Key_Space).toString()));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_Shift), "Shift"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_PageUp), QKeySequence(Qt::Key_PageUp).toString()));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_PageDown), QKeySequence(Qt::Key_PageDown).toString()));

        availableInputs.append(Input::NamedPair(makeInput(Qt::LeftButton), "LeftMouseClick"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::MiddleButton), "MiddleMouseClick"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::RightButton), "RightMouseClick"));
        
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_X_POS), "MouseMoveRight"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_X_NEG), "MouseMoveLeft"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_Y_POS), "MouseMoveUp"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_Y_NEG), "MouseMoveDown"));
        
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_WHEEL_Y_POS), "MouseWheelRight"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_WHEEL_Y_NEG), "MouseWheelLeft"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_WHEEL_X_POS), "MouseWheelUp"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_WHEEL_X_NEG), "MouseWheelDown"));
        
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_X_POS), "TouchpadRight"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_X_NEG), "TouchpadLeft"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_Y_POS), "TouchpadUp"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_Y_NEG), "TouchpadDown"));

        return availableInputs;
    };
}

QString KeyboardMouseDevice::getDefaultMappingConfig() {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/keyboardMouse.json";
    return MAPPING_JSON;
}

//void KeyboardMouseDevice::assignDefaultInputMapping(UserInputMapper& mapper) {
//    const float BUTTON_MOVE_SPEED = 1.0f;
//    const float BUTTON_YAW_SPEED = 0.75f;
//    const float BUTTON_PITCH_SPEED = 0.5f;
//    const float MOUSE_YAW_SPEED = 0.5f;
//    const float MOUSE_PITCH_SPEED = 0.25f;
//    const float TOUCH_YAW_SPEED = 0.5f;
//    const float TOUCH_PITCH_SPEED = 0.25f;
//    const float BUTTON_BOOM_SPEED = 0.1f;
// 
//    // AWSD keys mapping
//
//    mapper.addInputChannel(controller::BOOM_IN, makeInput(Qt::Key_E), makeInput(Qt::Key_Shift), BUTTON_BOOM_SPEED);
//    mapper.addInputChannel(controller::BOOM_OUT, makeInput(Qt::Key_C), makeInput(Qt::Key_Shift), BUTTON_BOOM_SPEED);
//    mapper.addInputChannel(controller::LATERAL_LEFT, makeInput(Qt::Key_A), makeInput(Qt::RightButton), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::LATERAL_RIGHT, makeInput(Qt::Key_D), makeInput(Qt::RightButton), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::LATERAL_LEFT, makeInput(Qt::Key_A), makeInput(Qt::Key_Shift), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::LATERAL_RIGHT, makeInput(Qt::Key_D), makeInput(Qt::Key_Shift), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::PITCH_DOWN, makeInput(Qt::Key_S), makeInput(Qt::Key_Shift), BUTTON_PITCH_SPEED);
//    mapper.addInputChannel(controller::PITCH_UP, makeInput(Qt::Key_W), makeInput(Qt::Key_Shift), BUTTON_PITCH_SPEED);
//
//    // Arrow keys mapping
//    mapper.addInputChannel(controller::LONGITUDINAL_BACKWARD, makeInput(Qt::Key_Down), BUTTON_MOVE_SPEED);
//    mapper.addInputChannel(controller::LONGITUDINAL_FORWARD, makeInput(Qt::Key_Up), BUTTON_MOVE_SPEED);
//    mapper.addInputChannel(controller::YAW_LEFT, makeInput(Qt::Key_Left), BUTTON_MOVE_SPEED);
//    mapper.addInputChannel(controller::YAW_RIGHT, makeInput(Qt::Key_Right), BUTTON_MOVE_SPEED);
//    mapper.addInputChannel(controller::VERTICAL_DOWN, makeInput(Qt::Key_PageDown), BUTTON_MOVE_SPEED);
//    mapper.addInputChannel(controller::VERTICAL_UP, makeInput(Qt::Key_PageUp), BUTTON_MOVE_SPEED);
//
//    mapper.addInputChannel(controller::LATERAL_LEFT, makeInput(Qt::Key_Left), makeInput(Qt::RightButton), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::LATERAL_RIGHT, makeInput(Qt::Key_Right), makeInput(Qt::RightButton), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::LATERAL_LEFT, makeInput(Qt::Key_Left), makeInput(Qt::Key_Shift), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::LATERAL_RIGHT, makeInput(Qt::Key_Right), makeInput(Qt::Key_Shift), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::PITCH_DOWN, makeInput(Qt::Key_Down), makeInput(Qt::Key_Shift), BUTTON_PITCH_SPEED);
//    mapper.addInputChannel(controller::PITCH_UP, makeInput(Qt::Key_Up), makeInput(Qt::Key_Shift), BUTTON_PITCH_SPEED);
//
//    // Mouse move
//    mapper.addInputChannel(controller::PITCH_DOWN, makeInput(MOUSE_AXIS_Y_NEG), makeInput(Qt::RightButton), MOUSE_PITCH_SPEED);
//    mapper.addInputChannel(controller::PITCH_UP, makeInput(MOUSE_AXIS_Y_POS), makeInput(Qt::RightButton), MOUSE_PITCH_SPEED);
//    mapper.addInputChannel(controller::YAW_LEFT, makeInput(MOUSE_AXIS_X_NEG), makeInput(Qt::RightButton), MOUSE_YAW_SPEED);
//    mapper.addInputChannel(controller::YAW_RIGHT, makeInput(MOUSE_AXIS_X_POS), makeInput(Qt::RightButton), MOUSE_YAW_SPEED);
//
//    
//#ifdef Q_OS_MAC
//    // wheel event modifier on Mac collide with the touchpad scroll event
//    mapper.addInputChannel(controller::PITCH_DOWN, makeInput(TOUCH_AXIS_Y_NEG), TOUCH_PITCH_SPEED);
//    mapper.addInputChannel(controller::PITCH_UP, makeInput(TOUCH_AXIS_Y_POS), TOUCH_PITCH_SPEED);
//    mapper.addInputChannel(controller::YAW_LEFT, makeInput(TOUCH_AXIS_X_NEG), TOUCH_YAW_SPEED);
//    mapper.addInputChannel(controller::YAW_RIGHT, makeInput(TOUCH_AXIS_X_POS), TOUCH_YAW_SPEED);
//#else
//    // Touch pad yaw pitch
//    mapper.addInputChannel(controller::PITCH_DOWN, makeInput(TOUCH_AXIS_Y_NEG), TOUCH_PITCH_SPEED);
//    mapper.addInputChannel(controller::PITCH_UP, makeInput(TOUCH_AXIS_Y_POS), TOUCH_PITCH_SPEED);
//    mapper.addInputChannel(controller::YAW_LEFT, makeInput(TOUCH_AXIS_X_NEG), TOUCH_YAW_SPEED);
//    mapper.addInputChannel(controller::YAW_RIGHT, makeInput(TOUCH_AXIS_X_POS), TOUCH_YAW_SPEED);
//    
//    // Wheel move
//    mapper.addInputChannel(controller::BOOM_IN, makeInput(MOUSE_AXIS_WHEEL_Y_POS), BUTTON_BOOM_SPEED);
//    mapper.addInputChannel(controller::BOOM_OUT, makeInput(MOUSE_AXIS_WHEEL_Y_NEG), BUTTON_BOOM_SPEED);
//    mapper.addInputChannel(controller::LATERAL_LEFT, makeInput(MOUSE_AXIS_WHEEL_X_NEG), BUTTON_YAW_SPEED);
//    mapper.addInputChannel(controller::LATERAL_RIGHT, makeInput(MOUSE_AXIS_WHEEL_X_POS), BUTTON_YAW_SPEED);
//
//#endif
//    
//    mapper.addInputChannel(controller::SHIFT, makeInput(Qt::Key_Space));
//    mapper.addInputChannel(controller::ACTION1, makeInput(Qt::Key_R));
//    mapper.addInputChannel(controller::ACTION2, makeInput(Qt::Key_T));
//}

