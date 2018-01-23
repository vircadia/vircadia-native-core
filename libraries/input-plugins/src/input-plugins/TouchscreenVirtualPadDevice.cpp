//
//  TouchscreenVirtualPadDevice.cpp
//  input-plugins/src/input-plugins
//
//  Created by Triplelexx on 01/31/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "TouchscreenVirtualPadDevice.h"
#include "KeyboardMouseDevice.h"

#include <QtGui/QTouchEvent>
#include <QGestureEvent>
#include <QGuiApplication>
#include <QWindow>
#include <QScreen>

#include <controllers/UserInputMapper.h>
#include <PathUtils.h>
#include <NumericalConstants.h>
#include "VirtualPadManager.h"

const char* TouchscreenVirtualPadDevice::NAME = "TouchscreenVirtualPad";

bool TouchscreenVirtualPadDevice::isSupported() const {
    for (auto touchDevice : QTouchDevice::devices()) {
        if (touchDevice->type() == QTouchDevice::TouchScreen) {
            return true;
        }
    }
#if defined(Q_OS_ANDROID)
    // last chance, assume that if this is android, a touchscreen is indeed supported
    return true;
#endif
    return false;
}

float clip(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}

void TouchscreenVirtualPadDevice::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData);
    });

    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (_validTouchLeft) {
        float leftDistanceScaleX, leftDistanceScaleY;
        leftDistanceScaleX = (_currentTouchLeftPoint.x - _firstTouchLeftPoint.x) / _screenDPIScale.x;
        leftDistanceScaleY = (_currentTouchLeftPoint.y - _firstTouchLeftPoint.y) / _screenDPIScale.y;

        leftDistanceScaleX = clip(leftDistanceScaleX, -STICK_RADIUS_INCHES, STICK_RADIUS_INCHES);
        leftDistanceScaleY = clip(leftDistanceScaleY, -STICK_RADIUS_INCHES, STICK_RADIUS_INCHES);

        // NOW BETWEEN -1 1
        leftDistanceScaleX /= STICK_RADIUS_INCHES;
        leftDistanceScaleY /= STICK_RADIUS_INCHES;

        _inputDevice->_axisStateMap[controller::LX] = leftDistanceScaleX;
        _inputDevice->_axisStateMap[controller::LY] = leftDistanceScaleY;

        /* Shared variables for stick rendering (clipped to the stick radius)*/
        // Prevent this for being done when not in first person view
        virtualPadManager.getLeftVirtualPad()->setBeingTouched(true);
        virtualPadManager.getLeftVirtualPad()->setFirstTouch(_firstTouchLeftPoint);
        virtualPadManager.getLeftVirtualPad()->setCurrentTouch(
                glm::vec2(clip(_currentTouchLeftPoint.x, -STICK_RADIUS_INCHES * _screenDPIScale.x + _firstTouchLeftPoint.x, STICK_RADIUS_INCHES * _screenDPIScale.x + _firstTouchLeftPoint.x),
                clip(_currentTouchLeftPoint.y, -STICK_RADIUS_INCHES * _screenDPIScale.y + _firstTouchLeftPoint.y, STICK_RADIUS_INCHES * _screenDPIScale.y + _firstTouchLeftPoint.y))
        );
    } else {
        virtualPadManager.getLeftVirtualPad()->setBeingTouched(false);
    }

    if (_validTouchRight) {
        float rightDistanceScaleX, rightDistanceScaleY;
        rightDistanceScaleX = (_currentTouchRightPoint.x - _firstTouchRightPoint.x) / _screenDPIScale.x;
        rightDistanceScaleY = (_currentTouchRightPoint.y - _firstTouchRightPoint.y) / _screenDPIScale.y;

        rightDistanceScaleX = clip(rightDistanceScaleX, -STICK_RADIUS_INCHES, STICK_RADIUS_INCHES);
        rightDistanceScaleY = clip(rightDistanceScaleY, -STICK_RADIUS_INCHES, STICK_RADIUS_INCHES);

        // NOW BETWEEN -1 1
        rightDistanceScaleX /= STICK_RADIUS_INCHES;
        rightDistanceScaleY /= STICK_RADIUS_INCHES;

        _inputDevice->_axisStateMap[controller::RX] = rightDistanceScaleX;
        _inputDevice->_axisStateMap[controller::RY] = rightDistanceScaleY;
    }

}

void TouchscreenVirtualPadDevice::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _axisStateMap.clear();
}

void TouchscreenVirtualPadDevice::InputDevice::focusOutEvent() {
}

void TouchscreenVirtualPadDevice::debugPoints(const QTouchEvent* event, QString who) {
    // convert the touch points into an average
    const QList<QTouchEvent::TouchPoint>& tPoints = event->touchPoints();
    QVector<glm::vec2> points;
    int touchPoints = tPoints.count();
    for (int i = 0; i < touchPoints; ++i) {
        glm::vec2 thisPoint(tPoints[i].pos().x(), tPoints[i].pos().y());
        points << thisPoint;
    }
    QScreen* eventScreen = event->window()->screen();
    int midScreenX = eventScreen->size().width()/2;
    int lefties = 0;
    int righties = 0;
    vec2 currentPoint;
    for (int i = 0; i < points.length(); i++) {
        currentPoint = points.at(i);
        if (currentPoint.x < midScreenX) {
            lefties++;
        } else {
            righties++;
        }
    }
}

void TouchscreenVirtualPadDevice::touchBeginEvent(const QTouchEvent* event) {
    // touch begin here is a big begin -> begins both pads? maybe it does nothing
    debugPoints(event, " BEGIN ++++++++++++++++");
    KeyboardMouseDevice::enableTouch(false);
    QScreen* eventScreen = event->window()->screen();
    _screenWidthCenter = eventScreen->size().width() / 2;
    if (_screenDPI != eventScreen->physicalDotsPerInch()) {
        _screenDPIScale.x = (float)eventScreen->physicalDotsPerInchX();
        _screenDPIScale.y = (float)eventScreen->physicalDotsPerInchY();
        _screenDPI = eventScreen->physicalDotsPerInch();
    }
}

void TouchscreenVirtualPadDevice::touchEndEvent(const QTouchEvent* event) {
    // touch end here is a big reset -> resets both pads
    _touchPointCount = 0;
    KeyboardMouseDevice::enableTouch(true);
    debugPoints(event, " END ----------------");
    touchLeftEnd();
    touchRightEnd();
    _inputDevice->_axisStateMap.clear();
}

void TouchscreenVirtualPadDevice::touchUpdateEvent(const QTouchEvent* event) {
    _touchPointCount = event->touchPoints().count();

    const QList<QTouchEvent::TouchPoint>& tPoints = event->touchPoints();
    bool leftTouchFound = false;
    bool rightTouchFound = false;
    for (int i = 0; i < _touchPointCount; ++i) {
        glm::vec2 thisPoint(tPoints[i].pos().x(), tPoints[i].pos().y());
        if (thisPoint.x < _screenWidthCenter) {
            if (!leftTouchFound) {
                leftTouchFound = true;
                if (!_validTouchLeft) {
                    touchLeftBegin(thisPoint);
                } else {
                    touchLeftUpdate(thisPoint);
                }
            }
        } else {
            if (!rightTouchFound) {
                rightTouchFound = true;
                if (!_validTouchRight) {
                    touchRightBegin(thisPoint);
                } else {
                    touchRightUpdate(thisPoint);
                }
            }
        }
    }
    if (!leftTouchFound) {
        touchLeftEnd();
    }
    if (!rightTouchFound) {
        touchRightEnd();
    }
}

void TouchscreenVirtualPadDevice::touchLeftBegin(glm::vec2 touchPoint) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (virtualPadManager.isEnabled()) {
        _firstTouchLeftPoint = touchPoint;
        _validTouchLeft = true;
    }
}

void TouchscreenVirtualPadDevice::touchLeftUpdate(glm::vec2 touchPoint) {
    _currentTouchLeftPoint = touchPoint;
}

void TouchscreenVirtualPadDevice::touchLeftEnd() {
    if (_validTouchLeft) { // do stuff once
        _validTouchLeft = false;
        _inputDevice->_axisStateMap[controller::LX] = 0;
        _inputDevice->_axisStateMap[controller::LY] = 0;
    }
}

void TouchscreenVirtualPadDevice::touchRightBegin(glm::vec2 touchPoint) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (virtualPadManager.isEnabled()) {
        _firstTouchRightPoint = touchPoint;
        _validTouchRight = true;
    }
}

void TouchscreenVirtualPadDevice::touchRightUpdate(glm::vec2 touchPoint) {
    _currentTouchRightPoint = touchPoint;
}

void TouchscreenVirtualPadDevice::touchRightEnd() {
    if (_validTouchRight) { // do stuff once
        _validTouchRight = false;
        _inputDevice->_axisStateMap[controller::RX] = 0;
        _inputDevice->_axisStateMap[controller::RY] = 0;
    }
}

void TouchscreenVirtualPadDevice::touchGestureEvent(const QGestureEvent* event) {
    if (QGesture* gesture = event->gesture(Qt::PinchGesture)) {
        QPinchGesture* pinch = static_cast<QPinchGesture*>(gesture);
        _pinchScale = pinch->totalScaleFactor();
    }
}

controller::Input::NamedVector TouchscreenVirtualPadDevice::InputDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs{
            makePair(LX, "LX"),
            makePair(LY, "LY"),
            makePair(RX, "RX"),
            makePair(RY, "RY")
    };
    return availableInputs;
}

QString TouchscreenVirtualPadDevice::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/touchscreenvirtualpad.json";
    return MAPPING_JSON;
}
