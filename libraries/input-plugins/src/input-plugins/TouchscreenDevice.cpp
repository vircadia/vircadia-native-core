//
//  TouchscreenDevice.cpp
//  input-plugins/src/input-plugins
//
//  Created by Triplelexx on 01/31/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TouchscreenDevice.h"
#include "KeyboardMouseDevice.h"

#include <QtGui/QTouchEvent>
#include <QGestureEvent>
#include <QGuiApplication>
#include <QScreen>

#include <controllers/UserInputMapper.h>
#include <PathUtils.h>
#include <NumericalConstants.h>

const QString TouchscreenDevice::NAME = "Touchscreen";

void TouchscreenDevice::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) {
    _inputDevice->update(deltaTime, inputCalibrationData, jointsCaptured);

    // at DPI 100 use these arbitrary values to divide dragging distance 
    static const float DPI_SCALE_X = glm::clamp((float)(qApp->primaryScreen()->physicalDotsPerInchX() / 100.0), 1.0f, 10.0f)
                       * 600.0f;
    static const float DPI_SCALE_Y = glm::clamp((float)(qApp->primaryScreen()->physicalDotsPerInchY() / 100.0), 1.0f, 10.0f)
                       * 200.0f;

    float distanceScaleX, distanceScaleY;
    if (_touchPointCount == 1) {
        if (_firstTouchVec.x < _currentTouchVec.x) {
            distanceScaleX = (_currentTouchVec.x - _firstTouchVec.x) / DPI_SCALE_X;
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_AXIS_X_POS).getChannel()] = distanceScaleX;
        } else if (_firstTouchVec.x > _currentTouchVec.x) {
            distanceScaleX = (_firstTouchVec.x - _currentTouchVec.x) / DPI_SCALE_X;
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_AXIS_X_NEG).getChannel()] = distanceScaleX;
        }
        // Y axis is inverted, positive is pointing up the screen
        if (_firstTouchVec.y > _currentTouchVec.y) {
            distanceScaleY = (_firstTouchVec.y - _currentTouchVec.y) / DPI_SCALE_Y;
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_AXIS_Y_POS).getChannel()] = distanceScaleY;
        } else if (_firstTouchVec.y < _currentTouchVec.y) {
            distanceScaleY = (_currentTouchVec.y - _firstTouchVec.y) / DPI_SCALE_Y;
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_AXIS_Y_NEG).getChannel()] = distanceScaleY;
        }
    }
}

void TouchscreenDevice::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) {
    _axisStateMap.clear();
}

void TouchscreenDevice::InputDevice::focusOutEvent() {
}

void TouchscreenDevice::touchBeginEvent(const QTouchEvent* event) {
    const QTouchEvent::TouchPoint& point = event->touchPoints().at(0);
    _firstTouchVec = glm::vec2(point.pos().x(), point.pos().y());
    KeyboardMouseDevice::enableMouse(false);
}

void TouchscreenDevice::touchEndEvent(const QTouchEvent* event) {
    _touchPointCount = 0;
    KeyboardMouseDevice::enableMouse(true);
}

void TouchscreenDevice::touchUpdateEvent(const QTouchEvent* event) {
    const QTouchEvent::TouchPoint& point = event->touchPoints().at(0);
    _currentTouchVec = glm::vec2(point.pos().x(), point.pos().y());
    _touchPointCount = event->touchPoints().count();
}

void TouchscreenDevice::touchGestureEvent(const QGestureEvent* event) {
    if (QGesture* gesture = event->gesture(Qt::PinchGesture)) {
        QPinchGesture* pinch = static_cast<QPinchGesture*>(gesture);
        qreal scaleFactor = pinch->totalScaleFactor();
        if (scaleFactor > _lastPinchScale && scaleFactor != 0) {
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_GESTURE_PINCH_POS).getChannel()] = 1.0f;
        } else if (scaleFactor != 0) {
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_GESTURE_PINCH_NEG).getChannel()] = 1.0f;
        }
        _lastPinchScale = scaleFactor;
    }
}

controller::Input TouchscreenDevice::InputDevice::makeInput(TouchscreenDevice::TouchAxisChannel axis) const {
    return controller::Input(_deviceID, axis, controller::ChannelType::AXIS);
}

controller::Input TouchscreenDevice::InputDevice::makeInput(TouchscreenDevice::TouchGestureAxisChannel gesture) const {
    return controller::Input(_deviceID, gesture, controller::ChannelType::AXIS);
}

controller::Input::NamedVector TouchscreenDevice::InputDevice::getAvailableInputs() const {
    using namespace controller;
    static QVector<Input::NamedPair> availableInputs;
    static std::once_flag once;
    std::call_once(once, [&] {
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_X_POS), "DragRight"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_X_NEG), "DragLeft"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_Y_POS), "DragUp"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_Y_NEG), "DragDown"));

        availableInputs.append(Input::NamedPair(makeInput(TOUCH_GESTURE_PINCH_POS), "GesturePinchOut"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_GESTURE_PINCH_NEG), "GesturePinchIn"));
    });
    return availableInputs;
}

QString TouchscreenDevice::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/touchscreen.json";
    return MAPPING_JSON;
}