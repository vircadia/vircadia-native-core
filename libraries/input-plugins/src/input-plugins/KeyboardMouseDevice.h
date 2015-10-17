//
//  KeyboardMouseDevice.h
//  input-plugins/src/input-plugins
//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_KeyboardMouseDevice_h
#define hifi_KeyboardMouseDevice_h

#include <QTouchEvent>
#include <chrono>
#include "InputDevice.h"
#include "InputPlugin.h"

class KeyboardMouseDevice : public InputPlugin, public InputDevice {
    Q_OBJECT
public:
    enum KeyboardChannel {
        KEYBOARD_FIRST = 0,
        KEYBOARD_LAST = 255,
        KEYBOARD_MASK = 0x00FF,
    };

    enum MouseButtonChannel {
        MOUSE_BUTTON_LEFT = KEYBOARD_LAST + 1,
        MOUSE_BUTTON_RIGHT,
        MOUSE_BUTTON_MIDDLE,
    };

    enum MouseAxisChannel {
        MOUSE_AXIS_X_POS = MOUSE_BUTTON_MIDDLE + 1,
        MOUSE_AXIS_X_NEG,
        MOUSE_AXIS_Y_POS,
        MOUSE_AXIS_Y_NEG,
        MOUSE_AXIS_WHEEL_Y_POS,
        MOUSE_AXIS_WHEEL_Y_NEG,
        MOUSE_AXIS_WHEEL_X_POS,
        MOUSE_AXIS_WHEEL_X_NEG,
    };
        
    enum TouchAxisChannel {
        TOUCH_AXIS_X_POS = MOUSE_AXIS_WHEEL_X_NEG + 1,
        TOUCH_AXIS_X_NEG,
        TOUCH_AXIS_Y_POS,
        TOUCH_AXIS_Y_NEG,
    };
        
    enum TouchButtonChannel {
        TOUCH_BUTTON_PRESS = TOUCH_AXIS_Y_NEG + 1,
    };

    KeyboardMouseDevice() : InputDevice("Keyboard") {}

    // Plugin functions
    virtual bool isSupported() const override { return true; }
    virtual bool isJointController() const override { return false; }
    const QString& getName() const override { return NAME; }

    virtual void activate() override {};
    virtual void deactivate() override {};

    virtual void pluginFocusOutEvent() override { focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, bool jointsCaptured) override { update(deltaTime, jointsCaptured); }

    // Device functions
    virtual void registerToUserInputMapper(UserInputMapper& mapper) override;
    virtual void assignDefaultInputMapping(UserInputMapper& mapper) override;
    virtual void update(float deltaTime, bool jointsCaptured) override;
    virtual void focusOutEvent() override;
 
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void mouseMoveEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mousePressEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID = 0);

    void touchBeginEvent(const QTouchEvent* event);
    void touchEndEvent(const QTouchEvent* event);
    void touchUpdateEvent(const QTouchEvent* event);

    void wheelEvent(QWheelEvent* event);
    
    // Let's make it easy for Qt because we assume we love Qt forever
    UserInputMapper::Input makeInput(Qt::Key code);
    UserInputMapper::Input makeInput(Qt::MouseButton code);
    UserInputMapper::Input makeInput(KeyboardMouseDevice::MouseAxisChannel axis);
    UserInputMapper::Input makeInput(KeyboardMouseDevice::TouchAxisChannel axis);
    UserInputMapper::Input makeInput(KeyboardMouseDevice::TouchButtonChannel button);

    static const QString NAME;

protected:
    QPoint _lastCursor;
    glm::vec2 _lastTouch;
    bool _isTouching = false;
    
    glm::vec2 evalAverageTouchPoints(const QList<QTouchEvent::TouchPoint>& points) const;
    std::chrono::high_resolution_clock _clock;
    std::chrono::high_resolution_clock::time_point _lastTouchTime;
};

#endif // hifi_KeyboardMouseDevice_h
