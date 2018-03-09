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

#include <cmath>

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

void TouchscreenVirtualPadDevice::init() {
    _fixedPosition = true; // This should be config

    QScreen* eventScreen = qApp->primaryScreen();
    if (_screenDPIProvided != eventScreen->physicalDotsPerInch()) {
        _screenWidthCenter = eventScreen->size().width() / 2;
        _screenDPIScale.x = (float)eventScreen->physicalDotsPerInchX();
        _screenDPIScale.y = (float)eventScreen->physicalDotsPerInchY();
        _screenDPIProvided = eventScreen->physicalDotsPerInch();
        _screenDPI = eventScreen->physicalDotsPerInch();

        _fixedRadius = _screenDPI * 256 / 534;
        _fixedRadiusForCalc = _fixedRadius - _screenDPI * 105 / 534; // 105 is the radius of the stick circle
    }

    auto& virtualPadManager = VirtualPad::Manager::instance();
    setupFixedCenter(virtualPadManager, true);

    if (_fixedPosition) {
        virtualPadManager.getLeftVirtualPad()->setShown(virtualPadManager.isEnabled() && !virtualPadManager.isHidden()); // Show whenever it's enabled
    }
}

void TouchscreenVirtualPadDevice::setupFixedCenter(VirtualPad::Manager& virtualPadManager, bool force) {
    if (!_fixedPosition) return;

    //auto& virtualPadManager = VirtualPad::Manager::instance();
    if (_extraBottomMargin == virtualPadManager.extraBottomMargin() && !force) return; // Our only criteria to decide a center change is the bottom margin

    _extraBottomMargin = virtualPadManager.extraBottomMargin();
    float margin = _screenDPI * 59 / 534; // 59px is for our 'base' of 534dpi (Pixel XL or Huawei Mate 9 Pro)
    QScreen* eventScreen = qApp->primaryScreen(); // do not call every time
    _fixedCenterPosition = glm::vec2( _fixedRadius + margin, eventScreen->size().height() - margin - _fixedRadius - _extraBottomMargin);

    _firstTouchLeftPoint = _fixedCenterPosition;
    virtualPadManager.getLeftVirtualPad()->setFirstTouch(_firstTouchLeftPoint);
}

float clip(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}

glm::vec2 TouchscreenVirtualPadDevice::clippedPointInCircle(float radius, glm::vec2 origin, glm::vec2 touchPoint) {
    float deltaX = touchPoint.x-origin.x;
    float deltaY = touchPoint.y-origin.y;

    float distance = sqrt(pow(deltaX,2)+pow(deltaY,2));

    // First case, inside the boundaires, just use the distance
    if (distance <= radius) {
        return touchPoint;
    }

    // Second case, purely vertical (avoid division by zero)
    if (deltaX == 0.0f) {
        return vec2(touchPoint.x, clip(touchPoint.y, origin.y-radius, origin.y+radius) );
    }

    // Third case, calculate point in circumference
    // line formula
    float m = deltaY/deltaX;
    float b = touchPoint.y - m * touchPoint.x;

    // quadtratic coefs of circumference and line intersection
    float qa = pow(m,2)+1;
    float qb = 2 * ( m * b - origin.x - origin.y * m );
    float qc = powf(origin.x, 2) - powf(radius,2) + b * b - 2 * b * origin.y + powf(origin.y, 2);

    float discr = qb * qb - 4 * qa * qc;
    float discrSign = deltaX>0?1.0:-1.0;

    float finalX = (- qb + discrSign * sqrtf(discr)) / (2 * qa);
    float finalY = m * finalX + b;

    return vec2(finalX, finalY);
}

void TouchscreenVirtualPadDevice::processInputUseCircleMethod(VirtualPad::Manager& virtualPadManager) {
    vec2 clippedPoint = clippedPointInCircle(_fixedRadiusForCalc, _firstTouchLeftPoint, _currentTouchLeftPoint);

    _inputDevice->_axisStateMap[controller::LX] = (clippedPoint.x - _firstTouchLeftPoint.x) / _fixedRadiusForCalc;
    _inputDevice->_axisStateMap[controller::LY] = (clippedPoint.y - _firstTouchLeftPoint.y) / _fixedRadiusForCalc;

    virtualPadManager.getLeftVirtualPad()->setFirstTouch(_firstTouchLeftPoint);
    virtualPadManager.getLeftVirtualPad()->setCurrentTouch(clippedPoint);
    virtualPadManager.getLeftVirtualPad()->setBeingTouched(true);
    virtualPadManager.getLeftVirtualPad()->setShown(true); // If touched, show in any mode (fixed joystick position or non-fixed)
}

void TouchscreenVirtualPadDevice::processInputUseSquareMethod(VirtualPad::Manager& virtualPadManager) {
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
    virtualPadManager.getLeftVirtualPad()->setFirstTouch(_firstTouchLeftPoint);
    virtualPadManager.getLeftVirtualPad()->setCurrentTouch(
            glm::vec2(clip(_currentTouchLeftPoint.x, -STICK_RADIUS_INCHES * _screenDPIScale.x + _firstTouchLeftPoint.x, STICK_RADIUS_INCHES * _screenDPIScale.x + _firstTouchLeftPoint.x),
            clip(_currentTouchLeftPoint.y, -STICK_RADIUS_INCHES * _screenDPIScale.y + _firstTouchLeftPoint.y, STICK_RADIUS_INCHES * _screenDPIScale.y + _firstTouchLeftPoint.y))
    );
    virtualPadManager.getLeftVirtualPad()->setBeingTouched(true);
    virtualPadManager.getLeftVirtualPad()->setShown(true); // If touched, show in any mode (fixed joystick position or non-fixed)
}

void TouchscreenVirtualPadDevice::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData);
    });

    auto& virtualPadManager = VirtualPad::Manager::instance();
    setupFixedCenter(virtualPadManager);

    if (_validTouchLeft) {
        processInputUseCircleMethod(virtualPadManager);
    } else {
        virtualPadManager.getLeftVirtualPad()->setBeingTouched(false);
        if (_fixedPosition) {
            virtualPadManager.getLeftVirtualPad()->setCurrentTouch(_fixedCenterPosition); // reset to the center
            virtualPadManager.getLeftVirtualPad()->setShown(virtualPadManager.isEnabled() && !virtualPadManager.isHidden()); // Show whenever it's enabled
        } else {
            virtualPadManager.getLeftVirtualPad()->setShown(false);
        }
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
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (!virtualPadManager.isEnabled()) {
        return;
    }
    KeyboardMouseDevice::enableTouch(false);
}

void TouchscreenVirtualPadDevice::touchEndEvent(const QTouchEvent* event) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (!virtualPadManager.isEnabled()) {
        touchLeftEnd();
        touchRightEnd();
        return;
    }
    // touch end here is a big reset -> resets both pads
    _touchPointCount = 0;
    KeyboardMouseDevice::enableTouch(true);
    debugPoints(event, " END ----------------");
    touchLeftEnd();
    touchRightEnd();
    _inputDevice->_axisStateMap.clear();
}

void TouchscreenVirtualPadDevice::touchUpdateEvent(const QTouchEvent* event) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (!virtualPadManager.isEnabled()) {
        touchLeftEnd();
        touchRightEnd();
        return;
    }
    _touchPointCount = event->touchPoints().count();

    const QList<QTouchEvent::TouchPoint>& tPoints = event->touchPoints();
    bool leftTouchFound = false;
    bool rightTouchFound = false;
    for (int i = 0; i < _touchPointCount; ++i) {
        glm::vec2 thisPoint(tPoints[i].pos().x(), tPoints[i].pos().y());
        if (_validTouchLeft) {
            leftTouchFound = true;
            touchLeftUpdate(thisPoint);
        } else if (touchLeftBeginPointIsValid(thisPoint)) {
            if (!leftTouchFound) {
                leftTouchFound = true;
                touchLeftBegin(thisPoint);
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

bool TouchscreenVirtualPadDevice::touchLeftBeginPointIsValid(glm::vec2 touchPoint) {
    if (_fixedPosition) {
        // inside circle
        return pow(touchPoint.x - _fixedCenterPosition.x,2.0) + pow(touchPoint.y - _fixedCenterPosition.y, 2.0) < pow(_fixedRadius, 2.0);
    } else {
        // left side
        return touchPoint.x < _screenWidthCenter;
    }
}

void TouchscreenVirtualPadDevice::touchLeftBegin(glm::vec2 touchPoint) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (virtualPadManager.isEnabled()) {
        if (_fixedPosition) {
            _firstTouchLeftPoint = _fixedCenterPosition;
        } else {
            _firstTouchLeftPoint = touchPoint;
        }
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
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (!virtualPadManager.isEnabled()) {
        return;
    }
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
