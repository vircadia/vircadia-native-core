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

#include <chrono>

#include <QtCore/QPoint>

#include <controllers/InputDevice.h>
#include "InputPlugin.h"

class QTouchEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class KeyboardMouseDevice : public InputPlugin {
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
        MOUSE_BUTTON_LEFT_CLICKED,
        MOUSE_BUTTON_RIGHT_CLICKED,
        MOUSE_BUTTON_MIDDLE_CLICKED,
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

    // Plugin functions
    bool isSupported() const override { return true; }
    const QString& getName() const override { return NAME; }

    bool isHandController() const override { return false; }

    void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void eraseMouseClicked();

    void touchBeginEvent(const QTouchEvent* event);
    void touchEndEvent(const QTouchEvent* event);
    void touchUpdateEvent(const QTouchEvent* event);

    void wheelEvent(QWheelEvent* event);

    static void enableTouch(bool enableTouch) { _enableTouch = enableTouch; }
    
    static const QString NAME;

protected:

    class InputDevice : public controller::InputDevice {
    public:
        InputDevice() : controller::InputDevice("Keyboard") {}
    private:
        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
        virtual void focusOutEvent() override;

        // Let's make it easy for Qt because we assume we love Qt forever
        controller::Input makeInput(Qt::Key code) const;
        controller::Input makeInput(Qt::MouseButton code, bool clicked = false) const;
        controller::Input makeInput(MouseAxisChannel axis) const;
        controller::Input makeInput(TouchAxisChannel axis) const;
        controller::Input makeInput(TouchButtonChannel button) const;

        friend class KeyboardMouseDevice;
    };

public:
    const std::shared_ptr<InputDevice>& getInputDevice() const { return _inputDevice; }

protected:
    QPoint _lastCursor;
    quint64 _mousePressTime;
    bool _mouseMoved;
    glm::vec2 _lastTouch;
    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };

    bool _isTouching = false;
    std::chrono::high_resolution_clock _clock;
    std::chrono::high_resolution_clock::time_point _lastTouchTime;

    static bool _enableTouch;
};

#endif // hifi_KeyboardMouseDevice_h
