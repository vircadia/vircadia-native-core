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
    _viewTouchUpdateCount = 0;

    resize();

    auto& virtualPadManager = VirtualPad::Manager::instance();

    if (_fixedPosition) {
        virtualPadManager.getLeftVirtualPad()->setShown(virtualPadManager.isEnabled() && !virtualPadManager.isHidden()); // Show whenever it's enabled
    }

    KeyboardMouseDevice::enableTouch(false); // Touch for view controls is managed by this plugin
}

void TouchscreenVirtualPadDevice::resize() {
    QScreen* eventScreen = qApp->primaryScreen();
    if (_screenDPIProvided != eventScreen->physicalDotsPerInch()) {
        _screenWidthCenter = eventScreen->availableSize().width() / 2;
        _screenDPIScale.x = (float)eventScreen->physicalDotsPerInchX();
        _screenDPIScale.y = (float)eventScreen->physicalDotsPerInchY();
        _screenDPIProvided = eventScreen->physicalDotsPerInch();
        _screenDPI = eventScreen->physicalDotsPerInch();

        _fixedRadius = _screenDPI * 0.5f * VirtualPad::Manager::BASE_DIAMETER_PIXELS / VirtualPad::Manager::DPI;
        _fixedRadiusForCalc = _fixedRadius - _screenDPI * VirtualPad::Manager::STICK_RADIUS_PIXELS / VirtualPad::Manager::DPI;

        _buttonRadius = _screenDPI * VirtualPad::Manager::BTN_TRIMMED_RADIUS_PIXELS / VirtualPad::Manager::DPI;
    }

    auto& virtualPadManager = VirtualPad::Manager::instance();
    setupControlsPositions(virtualPadManager, true);
}

void TouchscreenVirtualPadDevice::setupControlsPositions(VirtualPad::Manager& virtualPadManager, bool force) {
    if (_extraBottomMargin == virtualPadManager.extraBottomMargin() && !force) return; // Our only criteria to decide a center change is the bottom margin

    QScreen* eventScreen = qApp->primaryScreen(); // do not call every time
    _extraBottomMargin = virtualPadManager.extraBottomMargin();

    // Movement stick
    float margin = _screenDPI * VirtualPad::Manager::BASE_MARGIN_PIXELS / VirtualPad::Manager::DPI;
    _fixedCenterPosition = glm::vec2( _fixedRadius + margin, eventScreen->availableSize().height() - margin - _fixedRadius - _extraBottomMargin);
    _moveRefTouchPoint = _fixedCenterPosition;
    virtualPadManager.getLeftVirtualPad()->setFirstTouch(_moveRefTouchPoint);

    // Jump button
    float btnPixelSize = _screenDPI * VirtualPad::Manager::BTN_FULL_PIXELS / VirtualPad::Manager::DPI;
    float rightMargin = _screenDPI * VirtualPad::Manager::BTN_RIGHT_MARGIN_PIXELS / VirtualPad::Manager::DPI;
    float bottomMargin = _screenDPI * VirtualPad::Manager::BTN_BOTTOM_MARGIN_PIXELS/ VirtualPad::Manager::DPI;
    glm::vec2 jumpButtonPosition = glm::vec2( eventScreen->availableSize().width() - rightMargin - btnPixelSize, eventScreen->availableSize().height() - bottomMargin - _buttonRadius - _extraBottomMargin);
    glm::vec2 rbButtonPosition = glm::vec2( eventScreen->availableSize().width() - rightMargin - btnPixelSize, eventScreen->availableSize().height() - 2 * bottomMargin - 3 * _buttonRadius - _extraBottomMargin);

    // Avoid generating buttons in portrait mode
    if ( eventScreen->availableSize().width() > eventScreen->availableSize().height() && _buttonsManager.buttonsCount() == 0) {
        _buttonsManager.addButton(TouchscreenButton(JUMP, JUMP_BUTTON, _buttonRadius, jumpButtonPosition, _inputDevice ));
        _buttonsManager.addButton(TouchscreenButton(RB, RB_BUTTON, _buttonRadius, rbButtonPosition, _inputDevice ));

        virtualPadManager.setButtonPosition(VirtualPad::Manager::Button::JUMP, jumpButtonPosition);
        virtualPadManager.setButtonPosition(VirtualPad::Manager::Button::HANDSHAKE, rbButtonPosition);
    }

}

float clip(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}

glm::vec2 TouchscreenVirtualPadDevice::clippedPointInCircle(float radius, glm::vec2 origin, glm::vec2 touchPoint) {
    float deltaX = touchPoint.x - origin.x;
    float deltaY = touchPoint.y - origin.y;

    float distance = sqrt(pow(deltaX, 2) + pow(deltaY, 2));

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
    float m = deltaY / deltaX;
    float b = touchPoint.y - m * touchPoint.x;

    // quadtratic coefs of circumference and line intersection
    float qa = powf(m, 2.0f) + 1.0f;
    float qb = 2.0f * ( m * b - origin.x - origin.y * m);
    float qc = powf(origin.x, 2.0f) - powf(radius, 2.0f) + b * b - 2.0f * b * origin.y + powf(origin.y, 2.0f);

    float discr = qb * qb - 4.0f * qa * qc;
    float discrSign = deltaX > 0.0f ? 1.0f : - 1.0f;

    float finalX = (-qb + discrSign * sqrtf(discr)) / (2.0f * qa);
    float finalY = m * finalX + b;

    return vec2(finalX, finalY);
}

void TouchscreenVirtualPadDevice::processInputDeviceForMove(VirtualPad::Manager& virtualPadManager) {
    vec2 clippedPoint = clippedPointInCircle(_fixedRadiusForCalc, _moveRefTouchPoint, _moveCurrentTouchPoint);

    _inputDevice->_axisStateMap[controller::LX].value = (clippedPoint.x - _moveRefTouchPoint.x) / _fixedRadiusForCalc;
    _inputDevice->_axisStateMap[controller::LY].value = (clippedPoint.y - _moveRefTouchPoint.y) / _fixedRadiusForCalc;

    virtualPadManager.getLeftVirtualPad()->setFirstTouch(_moveRefTouchPoint);
    virtualPadManager.getLeftVirtualPad()->setCurrentTouch(clippedPoint);
    virtualPadManager.getLeftVirtualPad()->setBeingTouched(true);
    virtualPadManager.getLeftVirtualPad()->setShown(true); // If touched, show in any mode (fixed joystick position or non-fixed)
}

void TouchscreenVirtualPadDevice::processInputDeviceForView() {
    // We use average across how many times we've got touchUpdate events.
    // Using the average instead of the full deltaX and deltaY, makes deltaTime in MyAvatar dont't accelerate rotation when there is a low touchUpdate rate (heavier domains).
    // (Because it multiplies this input value by deltaTime (with a coefficient)).
    _inputDevice->_axisStateMap[controller::RX].value = 
        _viewTouchUpdateCount == 0 ? 0 : (_viewCurrentTouchPoint.x - _viewRefTouchPoint.x) / _viewTouchUpdateCount;
    _inputDevice->_axisStateMap[controller::RY].value = 
        _viewTouchUpdateCount == 0 ? 0 : (_viewCurrentTouchPoint.y - _viewRefTouchPoint.y) / _viewTouchUpdateCount;

    // after use, save last touch point as ref
    _viewRefTouchPoint = _viewCurrentTouchPoint;
    _viewTouchUpdateCount = 0;
}

void TouchscreenVirtualPadDevice::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData);
    });

    auto& virtualPadManager = VirtualPad::Manager::instance();
    setupControlsPositions(virtualPadManager);

    if (_moveHasValidTouch) {
        processInputDeviceForMove(virtualPadManager);
    } else {
        virtualPadManager.getLeftVirtualPad()->setBeingTouched(false);
        if (_fixedPosition) {
            virtualPadManager.getLeftVirtualPad()->setCurrentTouch(_fixedCenterPosition); // reset to the center
            virtualPadManager.getLeftVirtualPad()->setShown(virtualPadManager.isEnabled() && !virtualPadManager.isHidden()); // Show whenever it's enabled
        } else {
            virtualPadManager.getLeftVirtualPad()->setShown(false);
        }
    }

    if (_viewHasValidTouch) {
        processInputDeviceForView();
    }

}

void TouchscreenVirtualPadDevice::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _axisStateMap.clear();
}

bool TouchscreenVirtualPadDevice::InputDevice::triggerHapticPulse(float strength, float duration, uint16_t index) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    virtualPadManager.requestHapticFeedback((int) duration);
    return true;
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
    int midScreenX = eventScreen->availableSize().width()/2;
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
    if (!virtualPadManager.isEnabled() && !virtualPadManager.isHidden()) {
        return;
    }
}

void TouchscreenVirtualPadDevice::touchEndEvent(const QTouchEvent* event) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (!virtualPadManager.isEnabled() && !virtualPadManager.isHidden()) {
        moveTouchEnd();
        viewTouchEnd();
        _buttonsManager.endTouchForAll();
        return;
    }
    // touch end here is a big reset -> resets both pads
    _touchPointCount = 0;
    _unusedTouches.clear();
    debugPoints(event, " END ----------------");
    moveTouchEnd();
    viewTouchEnd();
    _buttonsManager.endTouchForAll();
    _inputDevice->_axisStateMap.clear();
    _inputDevice->_buttonPressedMap.clear();
}

void TouchscreenVirtualPadDevice::processUnusedTouches(std::map<int, TouchType> unusedTouchesInEvent) {
    std::vector<int> touchesToDelete;
    for (auto const& touchEntry : _unusedTouches) {
        if (!unusedTouchesInEvent.count(touchEntry.first)) {
            touchesToDelete.push_back(touchEntry.first);
        }
    }
    for (int touchToDelete : touchesToDelete) {
        _unusedTouches.erase(touchToDelete);
    }

    for (auto const& touchEntry : unusedTouchesInEvent) {
        if (!_unusedTouches.count(touchEntry.first)) {
            _unusedTouches[touchEntry.first] = touchEntry.second;
        }
    }

}

void TouchscreenVirtualPadDevice::touchUpdateEvent(const QTouchEvent* event) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (!virtualPadManager.isEnabled() && !virtualPadManager.isHidden()) {
        moveTouchEnd();
        viewTouchEnd();
        return;
    }
    _touchPointCount = event->touchPoints().count();

    const QList<QTouchEvent::TouchPoint>& tPoints = event->touchPoints();
    bool moveTouchFound = false;
    bool viewTouchFound = false;

    int idxMoveStartingPointCandidate = -1;
    int idxViewStartingPointCandidate = -1;

    _buttonsManager.resetEventValues();

    glm::vec2 thisPoint;
    int thisPointId;
    std::map<int, TouchType> unusedTouchesInEvent;

    for (int i = 0; i < _touchPointCount; ++i) {
        thisPoint.x = tPoints[i].pos().x();
        thisPoint.y = tPoints[i].pos().y();
        thisPointId = tPoints[i].id();

        if (!moveTouchFound && _moveHasValidTouch && _moveCurrentTouchId == thisPointId) {
            // valid if it's an ongoing touch
            moveTouchFound = true;
            moveTouchUpdate(thisPoint);
            continue;
        }

        if (!viewTouchFound && _viewHasValidTouch && _viewCurrentTouchId == thisPointId) {
            // valid if it's an ongoing touch
            viewTouchFound = true;
            viewTouchUpdate(thisPoint);
            continue;
        }

        if (_buttonsManager.processOngoingTouch(thisPoint, thisPointId)) {
            continue;
        }

        if (!moveTouchFound && idxMoveStartingPointCandidate == -1 && moveTouchBeginIsValid(thisPoint) &&
                (!_unusedTouches.count(thisPointId) || _unusedTouches[thisPointId] == MOVE )) {
            idxMoveStartingPointCandidate = i;
            continue;
        }

        if (!viewTouchFound && idxViewStartingPointCandidate == -1 && viewTouchBeginIsValid(thisPoint) &&
                (!_unusedTouches.count(thisPointId) || _unusedTouches[thisPointId] == VIEW )) {
            idxViewStartingPointCandidate = i;
            continue;
        }

        if (_buttonsManager.findStartingTouchPointCandidate(thisPoint, thisPointId, i, _unusedTouches)) {
            continue;
        }

        if (moveTouchBeginIsValid(thisPoint)) {
            unusedTouchesInEvent[thisPointId] = MOVE;
        } else if (viewTouchBeginIsValid(thisPoint))  {
            unusedTouchesInEvent[thisPointId] = VIEW;
        } else {
            _buttonsManager.saveUnusedTouches(unusedTouchesInEvent, thisPoint, thisPointId);
        }

    }

    processUnusedTouches(unusedTouchesInEvent);

    if (!moveTouchFound) {
        if (idxMoveStartingPointCandidate != -1) {
            _moveCurrentTouchId = tPoints[idxMoveStartingPointCandidate].id();
            _unusedTouches.erase(_moveCurrentTouchId);
            thisPoint.x = tPoints[idxMoveStartingPointCandidate].pos().x();
            thisPoint.y = tPoints[idxMoveStartingPointCandidate].pos().y();
            moveTouchBegin(thisPoint);
        } else {
            moveTouchEnd();
        }
    }
    if (!viewTouchFound) {
        if (idxViewStartingPointCandidate != -1) {
            _viewCurrentTouchId = tPoints[idxViewStartingPointCandidate].id();
            _unusedTouches.erase(_viewCurrentTouchId);
            thisPoint.x = tPoints[idxViewStartingPointCandidate].pos().x();
            thisPoint.y = tPoints[idxViewStartingPointCandidate].pos().y();
            viewTouchBegin(thisPoint);
        } else {
            viewTouchEnd();
        }
    }

    _buttonsManager.processBeginOrEnd(thisPoint, tPoints, _unusedTouches);

}

bool TouchscreenVirtualPadDevice::viewTouchBeginIsValid(glm::vec2 touchPoint) {
    return !moveTouchBeginIsValid(touchPoint) && _buttonsManager.touchBeginInvalidForAllButtons(touchPoint);
}

bool TouchscreenVirtualPadDevice::moveTouchBeginIsValid(glm::vec2 touchPoint) {
    if (_fixedPosition) {
        // inside circle
        return glm::distance2(touchPoint, _fixedCenterPosition) < _fixedRadius * _fixedRadius;
    } else {
        // left side
        return touchPoint.x < _screenWidthCenter;
    }
}

void TouchscreenVirtualPadDevice::moveTouchBegin(glm::vec2 touchPoint) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (virtualPadManager.isEnabled() && !virtualPadManager.isHidden()) {
        if (_fixedPosition) {
            _moveRefTouchPoint = _fixedCenterPosition;
        } else {
            _moveRefTouchPoint = touchPoint;
        }
        _moveCurrentTouchPoint = touchPoint;
        _moveHasValidTouch = true;
    }
}

void TouchscreenVirtualPadDevice::moveTouchUpdate(glm::vec2 touchPoint) {
    _moveCurrentTouchPoint = touchPoint;
}

void TouchscreenVirtualPadDevice::moveTouchEnd() {
    if (_moveHasValidTouch) { // do stuff once
        _moveHasValidTouch = false;
        _inputDevice->_axisStateMap[controller::LX].value = 0;
        _inputDevice->_axisStateMap[controller::LY].value = 0;
    }
}

void TouchscreenVirtualPadDevice::viewTouchBegin(glm::vec2 touchPoint) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (virtualPadManager.isEnabled() && !virtualPadManager.isHidden()) {
        _viewRefTouchPoint = touchPoint;
        _viewCurrentTouchPoint = touchPoint;
        _viewTouchUpdateCount++;
        _viewHasValidTouch = true;
    }
}

void TouchscreenVirtualPadDevice::viewTouchUpdate(glm::vec2 touchPoint) {
    _viewCurrentTouchPoint = touchPoint;
    _viewTouchUpdateCount++;
}

void TouchscreenVirtualPadDevice::viewTouchEnd() {
    if (_viewHasValidTouch) { // do stuff once
        _viewHasValidTouch = false;
        _inputDevice->_axisStateMap[controller::RX].value = 0;
        _inputDevice->_axisStateMap[controller::RY].value = 0;
    }
}

void TouchscreenVirtualPadDevice::touchGestureEvent(const QGestureEvent* event) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (!virtualPadManager.isEnabled()  && !virtualPadManager.isHidden()) {
        return;
    }
    if (QGesture* gesture = event->gesture(Qt::PinchGesture)) {
        QPinchGesture* pinch = static_cast<QPinchGesture*>(gesture);
        _pinchScale = pinch->totalScaleFactor();
    }
}

controller::Input TouchscreenVirtualPadDevice::InputDevice::makeInput(TouchscreenVirtualPadDevice::TouchAxisChannel axis) const {
    return controller::Input(_deviceID, axis, controller::ChannelType::AXIS);
}

controller::Input TouchscreenVirtualPadDevice::InputDevice::makeInput(TouchscreenVirtualPadDevice::TouchButtonChannel button) const {
    return controller::Input(_deviceID, button, controller::ChannelType::BUTTON);
}

controller::Input::NamedVector TouchscreenVirtualPadDevice::InputDevice::getAvailableInputs() const {
    using namespace controller;
    QVector<Input::NamedPair> availableInputs{
        Input::NamedPair(makeInput(TouchAxisChannel::LX), "LX"),
        Input::NamedPair(makeInput(TouchAxisChannel::LY), "LY"),
        Input::NamedPair(makeInput(TouchAxisChannel::RX), "RX"),
        Input::NamedPair(makeInput(TouchAxisChannel::RY), "RY"),
        Input::NamedPair(makeInput(TouchButtonChannel::JUMP), "JUMP_BUTTON_PRESS"),
        Input::NamedPair(makeInput(TouchButtonChannel::RB), "RB")
    };
    return availableInputs;
}

QString TouchscreenVirtualPadDevice::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/touchscreenvirtualpad.json";
    return MAPPING_JSON;
}

TouchscreenVirtualPadDevice::TouchscreenButton::TouchscreenButton(
        TouchscreenVirtualPadDevice::TouchButtonChannel channelIn,
        TouchscreenVirtualPadDevice::TouchType touchTypeIn, float buttonRadiusIn,
        glm::vec2 buttonPositionIn, std::shared_ptr<InputDevice> inputDeviceIn) :
    buttonPosition(buttonPositionIn),
    buttonRadius(buttonRadiusIn),
    touchType(touchTypeIn),
    channel(channelIn),
    _inputDevice(inputDeviceIn)
{
}

void TouchscreenVirtualPadDevice::TouchscreenButton::touchBegin(glm::vec2 touchPoint) {
    auto& virtualPadManager = VirtualPad::Manager::instance();
    if (virtualPadManager.isEnabled() && !virtualPadManager.isHidden()) {
        hasValidTouch = true;

        _inputDevice->_buttonPressedMap.insert(channel);
    }
}

void TouchscreenVirtualPadDevice::TouchscreenButton::touchUpdate(glm::vec2 touchPoint) {

}

void TouchscreenVirtualPadDevice::TouchscreenButton::touchEnd() {
    if (hasValidTouch) {
        hasValidTouch = false;

        _inputDevice->_buttonPressedMap.erase(channel);
    }
}

bool TouchscreenVirtualPadDevice::TouchscreenButton::touchBeginIsValid(glm::vec2 touchPoint) {
    return glm::distance2(touchPoint, buttonPosition) < buttonRadius * buttonRadius;
}

void TouchscreenVirtualPadDevice::TouchscreenButton::resetEventValues() {
    _candidatePointIdx = -1;
    _found = false;
}

TouchscreenVirtualPadDevice::TouchscreenButtonsManager::TouchscreenButtonsManager() {}

void TouchscreenVirtualPadDevice::TouchscreenButtonsManager::addButton(
        TouchscreenVirtualPadDevice::TouchscreenButton button) {
    buttons.push_back(button);
}

void TouchscreenVirtualPadDevice::TouchscreenButtonsManager::resetEventValues() {
    for(int i = 0; i < buttons.size(); i++) {
        TouchscreenButton &button = buttons[i];
        button.resetEventValues();
    }
}

bool
TouchscreenVirtualPadDevice::TouchscreenButtonsManager::processOngoingTouch(glm::vec2 thisPoint,
                                                                            int thisPointId) {
    for(int i = 0; i < buttons.size(); i++) {
        TouchscreenButton &button = buttons[i];

        if (!button._found && button.hasValidTouch && button.currentTouchId == thisPointId) {
            // valid if it's an ongoing touch
            button._found = true;
            button.touchUpdate(thisPoint);
            return true;
        }
    }
    return false;

}

bool TouchscreenVirtualPadDevice::TouchscreenButtonsManager::findStartingTouchPointCandidate(
        glm::vec2 thisPoint, int thisPointId, int thisPointIdx, std::map<int, TouchType> &globalUnusedTouches) {

    for(int i = 0; i < buttons.size(); i++) {
        TouchscreenButton &button = buttons[i];
        if (!button._found && button._candidatePointIdx == -1 && button.touchBeginIsValid(thisPoint)) {
            if (!globalUnusedTouches.count(thisPointId) ) {
                button._candidatePointIdx = thisPointIdx;
                return true;
            } else if (globalUnusedTouches[thisPointId] == button.touchType) {
                button._candidatePointIdx = thisPointIdx;
                return true;
            }
        }
    }
    return false;

}

void TouchscreenVirtualPadDevice::TouchscreenButtonsManager::saveUnusedTouches(
        std::map<int, TouchscreenVirtualPadDevice::TouchType> &unusedTouchesInEvent, glm::vec2 thisPoint,
        int thisPointId) {
    for(int i = 0; i < buttons.size(); i++) {
        TouchscreenButton &button = buttons[i];
        if (button.touchBeginIsValid(thisPoint)) {
            unusedTouchesInEvent[thisPointId] = button.touchType;
            return;
        }
    }

}

void TouchscreenVirtualPadDevice::TouchscreenButtonsManager::processBeginOrEnd(
        glm::vec2 thisPoint, const QList<QTouchEvent::TouchPoint> &tPoints, std::map<int, TouchType> globalUnusedTouches) {
    for(int i = 0; i < buttons.size(); i++) {
        TouchscreenButton &button = buttons[i];
        if (!button._found) {
            if (button._candidatePointIdx != -1) {
                button.currentTouchId = tPoints[button._candidatePointIdx].id();
                globalUnusedTouches.erase(button.currentTouchId);
                thisPoint.x = tPoints[button._candidatePointIdx].pos().x();
                thisPoint.y = tPoints[button._candidatePointIdx].pos().y();
                button.touchBegin(thisPoint);
            } else {
                if (button.hasValidTouch) {
                    button.touchEnd();
                }
            }
        }
    }

}

void TouchscreenVirtualPadDevice::TouchscreenButtonsManager::endTouchForAll() {
    for(int i = 0; i < buttons.size(); i++) {
        TouchscreenButton &button = buttons[i];
        button.touchEnd();
    }
}

bool TouchscreenVirtualPadDevice::TouchscreenButtonsManager::touchBeginInvalidForAllButtons(glm::vec2 touchPoint) {
    for(int i = 0; i < buttons.size(); i++) {
        TouchscreenButton &button = buttons[i];
        if (button.touchBeginIsValid(touchPoint)) {
            return false;
        }
    }
    return true;
}
