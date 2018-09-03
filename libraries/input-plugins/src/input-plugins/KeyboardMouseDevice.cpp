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
#include <NumericalConstants.h>

const char* KeyboardMouseDevice::NAME = "Keyboard/Mouse";
bool KeyboardMouseDevice::_enableTouch = true;

void KeyboardMouseDevice::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData);
        eraseMouseClicked();

        _inputDevice->_axisStateMap[MOUSE_AXIS_X] = _lastCursor.x();
        _inputDevice->_axisStateMap[MOUSE_AXIS_Y] = _lastCursor.y();
    });

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

void KeyboardMouseDevice::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    _axisStateMap.clear();
}

void KeyboardMouseDevice::InputDevice::focusOutEvent() {
    _buttonPressedMap.clear();
}

void KeyboardMouseDevice::keyPressEvent(QKeyEvent* event) {
    auto input = _inputDevice->makeInput((Qt::Key) event->key());
    auto result = _inputDevice->_buttonPressedMap.insert(input.getChannel());
    if (result.second) {
        // key pressed again ? without catching the release event ?
    }
}

void KeyboardMouseDevice::keyReleaseEvent(QKeyEvent* event) {
    auto input = _inputDevice->makeInput((Qt::Key) event->key());
    _inputDevice->_buttonPressedMap.erase(input.getChannel());
}

void KeyboardMouseDevice::mousePressEvent(QMouseEvent* event) {
    auto input = _inputDevice->makeInput((Qt::MouseButton) event->button());
    auto result = _inputDevice->_buttonPressedMap.insert(input.getChannel());
    if (!result.second) {
        // key pressed again ? without catching the release event ?
    }
    _lastCursor = event->pos();
    _mousePressTime = usecTimestampNow();
    _mouseMoved = false;

    _mousePressPos = event->pos();
    _clickDeadspotActive = true;
}

void KeyboardMouseDevice::mouseReleaseEvent(QMouseEvent* event) {
    auto input = _inputDevice->makeInput((Qt::MouseButton) event->button());
    _inputDevice->_buttonPressedMap.erase(input.getChannel());

    // if we pressed and released at the same location within a small time window, then create a "_CLICKED" 
    // input for this button we might want to add some small tolerance to this so if you do a small drag it 
    // still counts as a click.
    static const int CLICK_TIME = USECS_PER_MSEC * 500; // 500 ms to click
    if (_clickDeadspotActive && (usecTimestampNow() - _mousePressTime < CLICK_TIME)) {
        _inputDevice->_buttonPressedMap.insert(_inputDevice->makeInput((Qt::MouseButton) event->button(), true).getChannel());
    }

    _clickDeadspotActive = false;
}

void KeyboardMouseDevice::eraseMouseClicked() {
    _inputDevice->_buttonPressedMap.erase(_inputDevice->makeInput(Qt::LeftButton, true).getChannel());
    _inputDevice->_buttonPressedMap.erase(_inputDevice->makeInput(Qt::MiddleButton, true).getChannel());
    _inputDevice->_buttonPressedMap.erase(_inputDevice->makeInput(Qt::RightButton, true).getChannel());
}

void KeyboardMouseDevice::mouseMoveEvent(QMouseEvent* event) {
    QPoint currentPos = event->pos();
    QPoint currentMove = currentPos - _lastCursor;

    _inputDevice->_axisStateMap[MOUSE_AXIS_X_POS] = (currentMove.x() > 0 ? currentMove.x() : 0.0f);
    _inputDevice->_axisStateMap[MOUSE_AXIS_X_NEG] = (currentMove.x() < 0 ? -currentMove.x() : 0.0f);
    // Y mouse is inverted positive is pointing up the screen
    _inputDevice->_axisStateMap[MOUSE_AXIS_Y_POS] = (currentMove.y() < 0 ? -currentMove.y() : 0.0f);
    _inputDevice->_axisStateMap[MOUSE_AXIS_Y_NEG] = (currentMove.y() > 0 ? currentMove.y() : 0.0f);

    // FIXME - this has the characteristic that it will show large jumps when you move the cursor
    // outside of the application window, because we don't get MouseEvents when the cursor is outside
    // of the application window.
    _lastCursor = currentPos;

    _mouseMoved = true;

    const int CLICK_EVENT_DEADSPOT = 6; // pixels
    if (_clickDeadspotActive && (_mousePressPos - currentPos).manhattanLength() > CLICK_EVENT_DEADSPOT) {
        _clickDeadspotActive = false;
    }
}

void KeyboardMouseDevice::wheelEvent(QWheelEvent* event) {
    auto currentMove = event->angleDelta() / 120.0f;

    _inputDevice->_axisStateMap[_inputDevice->makeInput(MOUSE_AXIS_WHEEL_X_POS).getChannel()] = (currentMove.x() > 0 ? currentMove.x() : 0.0f);
    _inputDevice->_axisStateMap[_inputDevice->makeInput(MOUSE_AXIS_WHEEL_X_NEG).getChannel()] = (currentMove.x() < 0 ? -currentMove.x() : 0.0f);
    _inputDevice->_axisStateMap[_inputDevice->makeInput(MOUSE_AXIS_WHEEL_Y_POS).getChannel()] = (currentMove.y() > 0 ? currentMove.y() : 0.0f);
    _inputDevice->_axisStateMap[_inputDevice->makeInput(MOUSE_AXIS_WHEEL_Y_NEG).getChannel()] = (currentMove.y() < 0 ? -currentMove.y() : 0.0f);
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
    if (_enableTouch) {
        _isTouching = event->touchPointStates().testFlag(Qt::TouchPointPressed);
        _lastTouch = evalAverageTouchPoints(event->touchPoints());
        _lastTouchTime = _clock.now();
    }
}

void KeyboardMouseDevice::touchEndEvent(const QTouchEvent* event) {
    if (_enableTouch) {
        _isTouching = false;
        _lastTouch = evalAverageTouchPoints(event->touchPoints());
        _lastTouchTime = _clock.now();
    }
}

void KeyboardMouseDevice::touchUpdateEvent(const QTouchEvent* event) {
    if (_enableTouch) {
        auto currentPos = evalAverageTouchPoints(event->touchPoints());
        _lastTouchTime = _clock.now();

        if (!_isTouching) {
            _isTouching = event->touchPointStates().testFlag(Qt::TouchPointPressed);
        } else {
            auto currentMove = currentPos - _lastTouch;

            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_AXIS_X_POS).getChannel()] = (currentMove.x > 0 ? currentMove.x : 0.0f);
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_AXIS_X_NEG).getChannel()] = (currentMove.x < 0 ? -currentMove.x : 0.0f);
            // Y mouse is inverted positive is pointing up the screen
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_AXIS_Y_POS).getChannel()] = (currentMove.y < 0 ? -currentMove.y : 0.0f);
            _inputDevice->_axisStateMap[_inputDevice->makeInput(TOUCH_AXIS_Y_NEG).getChannel()] = (currentMove.y > 0 ? currentMove.y : 0.0f);
        }

        _lastTouch = currentPos;
    }
}

controller::Input KeyboardMouseDevice::InputDevice::makeInput(Qt::Key code) const {
    auto shortCode = (uint16_t)(code & KEYBOARD_MASK);
    if (shortCode != code) {
       shortCode |= 0x0800; // add this bit instead of the way Qt::Key add a bit on the 3rd byte for some keys
    }
    return controller::Input(_deviceID, shortCode, controller::ChannelType::BUTTON);
}

controller::Input KeyboardMouseDevice::InputDevice::makeInput(Qt::MouseButton code, bool clicked) const {
    switch (code) {
        case Qt::LeftButton:
            return controller::Input(_deviceID, clicked ? MOUSE_BUTTON_LEFT_CLICKED :
                                                MOUSE_BUTTON_LEFT, controller::ChannelType::BUTTON);
        case Qt::RightButton:
            return controller::Input(_deviceID, clicked ? MOUSE_BUTTON_RIGHT_CLICKED :
                                                MOUSE_BUTTON_RIGHT, controller::ChannelType::BUTTON);
        case Qt::MiddleButton:
            return controller::Input(_deviceID, clicked ? MOUSE_BUTTON_MIDDLE_CLICKED :
                                                MOUSE_BUTTON_MIDDLE, controller::ChannelType::BUTTON);
        default:
            return controller::Input();
    };
}

controller::Input KeyboardMouseDevice::InputDevice::makeInput(KeyboardMouseDevice::MouseAxisChannel axis) const {
    return controller::Input(_deviceID, axis, controller::ChannelType::AXIS);
}

controller::Input KeyboardMouseDevice::InputDevice::makeInput(KeyboardMouseDevice::TouchAxisChannel axis) const {
    return controller::Input(_deviceID, axis, controller::ChannelType::AXIS);
}

controller::Input KeyboardMouseDevice::InputDevice::makeInput(KeyboardMouseDevice::TouchButtonChannel button) const {
    return controller::Input(_deviceID, button, controller::ChannelType::BUTTON);
}

/**jsdoc
 * <p>The <code>Controller.Hardware.Keyboard</code> object has properties representing keyboard, mouse, and display touch 
 * events. The property values are integer IDs, uniquely identifying each output. <em>Read-only.</em> These can be mapped to 
 * actions or functions or <code>Controller.Standard</code> items in a {@link RouteObject} mapping. For presses, each data 
 * value is either <code>1.0</code> for "true" or <code>0.0</code> for "false".</p>
 * <table>
 *   <thead>
 *     <tr><th>Property</th><th>Type</th><td>Data</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code> &ndash; <code>9</code></td><td>number</td><td>number</td><td>A "0" &ndash; "1" key on the 
 *       keyboard or keypad is pressed.</td></tr>
 *     <tr><td><code>A</code> &ndash; <code>Z</code></td><td>number</td><td>number</td><td>A "A" &ndash; "Z" key on the 
 *       keyboard is pressed.</td></tr>
 *     <tr><td><code>Space</code></td><td>number</td><td>number</td><td>The space bar on the keyboard is pressed.</td></tr>
 *     <tr><td><code>Tab</code></td><td>number</td><td>number</td><td>The tab key on the keyboard is pressed.</td></tr>
 *     <tr><td><code>Shift</code></td><td>number</td><td>number</td><td>The shift key on the keyboard is pressed.</td></tr>
 *     <tr><td><code>Control</code></td><td>number</td><td>number</td><td>The control key on the keyboard is pressed. (The 
 *       "Command" key on OSX.)</td></tr>
 *     <tr><td><code>Left</code></td><td>number</td><td>number</td><td>The left arrow key on the keyboard or keypad is pressed.
 *       </td></tr>
 *     <tr><td><code>Right</code></td><td>number</td><td>number</td><td>The right arrow key on the keyboard or keypad is 
 *       pressed.</td></tr>
 *     <tr><td><code>Up</code></td><td>number</td><td>number</td><td>The up arrow key on the keyboard or keypad is pressed.
 *       </td></tr>
 *     <tr><td><code>Down</code></td><td>number</td><td>number</td><td>The down arrow key on the keyboard or keypad is pressed.
 *       </td></tr>
 *     <tr><td><code>PgUp</code></td><td>number</td><td>number</td><td>The page up key on the keyboard or keypad is pressed.
 *       </td></tr>
 *     <tr><td><code>PgDown</code></td><td>number</td><td>number</td><td>The page down key on the keyboard or keypad is pressed.
 *       </td></tr>
 *     <tr><td><code>LeftMouseButton</code></td><td>number</td><td>number</td><td>The left mouse button pressed.</td></tr>
 *     <tr><td><code>MiddleMouseButton</code></td><td>number</td><td>number</td><td>The middle mouse button pressed.</td></tr>
 *     <tr><td><code>RightMouseButton</code></td><td>number</td><td>number</td><td>The right mouse button pressed.</td></tr>
 *     <tr><td><code>LeftMouseClicked</code></td><td>number</td><td>number</td><td>The left mouse button clicked.</td></tr>
 *     <tr><td><code>MiddleMouseClicked</code></td><td>number</td><td>number</td><td>The middle mouse button clicked.</td></tr>
 *     <tr><td><code>RightMouseClicked</code></td><td>number</td><td>number</td><td>The right mouse button clicked.</td></tr>
 *     <tr><td><code>MouseMoveRight</code></td><td>number</td><td>number</td><td>The mouse moved right.</td></tr>
 *     <tr><td><code>MouseMoveLeft</code></td><td>number</td><td>number</td><td>The mouse moved left.</td></tr>
 *     <tr><td><code>MouseMoveUp</code></td><td>number</td><td>number</td><td>The mouse moved up.</td></tr>
 *     <tr><td><code>MouseMoveDown</code></td><td>number</td><td>number</td><td>The mouse moved down.</td></tr>
 *     <tr><td><code>MouseX</code></td><td>number</td><td>number</td><td>The mouse x-coordinate changed. The data value is its 
 *       new x-coordinate value.</td></tr>
 *     <tr><td><code>MouseY</code></td><td>number</td><td>number</td><td>The mouse y-coordinate changed. The data value is its 
 *       new y-coordinate value.</td></tr>
 *     <tr><td><code>MouseWheelRight</code></td><td>number</td><td>number</td><td>The mouse wheel rotated left. The data value 
 *       is the number of units rotated (typically <code>1.0</code>).</td></tr>
 *     <tr><td><code>MouseWheelLeft</code></td><td>number</td><td>number</td><td>The mouse wheel rotated left. The data value 
 *       is the number of units rotated (typically <code>1.0</code>).</td></tr>
 *     <tr><td><code>MouseWheelUp</code></td><td>number</td><td>number</td><td>The mouse wheel rotated up. The data value 
 *       is the number of units rotated (typically <code>1.0</code>).</td></tr>
 *     <tr><td><code>MouseWheelDown</code></td><td>number</td><td>number</td><td>The mouse wheel rotated down. The data value 
 *       is the number of units rotated (typically <code>1.0</code>).</td></tr>
  *     <tr><td><code>TouchpadRight</code></td><td>number</td><td>number</td><td>The average touch on a touch-enabled device 
 *       moved right. The data value is how far the average position of all touch points moved.</td></tr>
 *     <tr><td><code>TouchpadLeft</code></td><td>number</td><td>number</td><td>The average touch on a touch-enabled device 
 *       moved left. The data value is how far the average position of all touch points moved.</td></tr>
 *     <tr><td><code>TouchpadUp</code></td><td>number</td><td>number</td><td>The average touch on a touch-enabled device 
 *       moved up. The data value is how far the average position of all touch points moved.</td></tr>
 *     <tr><td><code>TouchpadDown</code></td><td>number</td><td>number</td><td>The average touch on a touch-enabled device 
 *       moved down. The data value is how far the average position of all touch points moved.</td></tr>
  *   </tbody>
 * </table>
 * @typedef {object} Controller.Hardware-Keyboard
 * @todo <em>Currently, the mouse wheel in an ordinary mouse generates left/right wheel events instead of up/down.</em>
 */
controller::Input::NamedVector KeyboardMouseDevice::InputDevice::getAvailableInputs() const {
    using namespace controller;
    static QVector<Input::NamedPair> availableInputs;
    static std::once_flag once;
    std::call_once(once, [&] {
        for (int i = (int)Qt::Key_0; i <= (int)Qt::Key_9; i++) {
            availableInputs.append(Input::NamedPair(makeInput(Qt::Key(i)), QKeySequence(Qt::Key(i)).toString()));
        }
        for (int i = (int)Qt::Key_A; i <= (int)Qt::Key_Z; i++) {
            availableInputs.append(Input::NamedPair(makeInput(Qt::Key(i)), QKeySequence(Qt::Key(i)).toString()));
        }
        for (int i = (int)Qt::Key_Left; i <= (int)Qt::Key_Down; i++) {
            availableInputs.append(Input::NamedPair(makeInput(Qt::Key(i)), QKeySequence(Qt::Key(i)).toString()));
        }
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_Space), QKeySequence(Qt::Key_Space).toString()));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_Shift), "Shift"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_PageUp), QKeySequence(Qt::Key_PageUp).toString()));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_PageDown), QKeySequence(Qt::Key_PageDown).toString()));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_Tab), QKeySequence(Qt::Key_Tab).toString()));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_Control), "Control"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_Delete), "Delete"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::Key_Backspace), QKeySequence(Qt::Key_Backspace).toString()));

        availableInputs.append(Input::NamedPair(makeInput(Qt::LeftButton), "LeftMouseButton"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::MiddleButton), "MiddleMouseButton"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::RightButton), "RightMouseButton"));

        availableInputs.append(Input::NamedPair(makeInput(Qt::LeftButton, true), "LeftMouseClicked"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::MiddleButton, true), "MiddleMouseClicked"));
        availableInputs.append(Input::NamedPair(makeInput(Qt::RightButton, true), "RightMouseClicked"));

        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_X_POS), "MouseMoveRight"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_X_NEG), "MouseMoveLeft"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_Y_POS), "MouseMoveUp"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_Y_NEG), "MouseMoveDown"));

        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_X), "MouseX"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_Y), "MouseY"));

        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_WHEEL_Y_POS), "MouseWheelRight"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_WHEEL_Y_NEG), "MouseWheelLeft"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_WHEEL_X_POS), "MouseWheelUp"));
        availableInputs.append(Input::NamedPair(makeInput(MOUSE_AXIS_WHEEL_X_NEG), "MouseWheelDown"));

        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_X_POS), "TouchpadRight"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_X_NEG), "TouchpadLeft"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_Y_POS), "TouchpadUp"));
        availableInputs.append(Input::NamedPair(makeInput(TOUCH_AXIS_Y_NEG), "TouchpadDown"));
    });
    return availableInputs;
}

QString KeyboardMouseDevice::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/keyboardMouse.json";
    return MAPPING_JSON;
}
