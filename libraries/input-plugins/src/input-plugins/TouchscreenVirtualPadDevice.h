//
//  TouchscreenVirtualPadDevice.h
//  input-plugins/src/input-plugins
//
//  Created by Triplelexx on 1/31/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TouchscreenVirtualPadDevice_h
#define hifi_TouchscreenVirtualPadDevice_h

#include <controllers/InputDevice.h>
#include "InputPlugin.h"
#include <QtGui/qtouchdevice.h>
#include "VirtualPadManager.h"

class QTouchEvent;
class QGestureEvent;

class TouchscreenVirtualPadDevice : public InputPlugin {
Q_OBJECT
public:

    // Plugin functions
    virtual void init() override;
    virtual void resize();
    virtual bool isSupported() const override;
    virtual const QString getName() const override { return NAME; }

    bool isHandController() const override { return false; }

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;

    void touchBeginEvent(const QTouchEvent* event);
    void touchEndEvent(const QTouchEvent* event);
    void touchUpdateEvent(const QTouchEvent* event);
    void touchGestureEvent(const QGestureEvent* event);

    static const char* NAME;

    int _viewTouchUpdateCount;
    enum TouchAxisChannel {
        LX,
        LY,
        RX,
        RY
    };

    enum TouchButtonChannel {
        JUMP_BUTTON_PRESS
    };

protected:

    class InputDevice : public controller::InputDevice {
    public:
        InputDevice() : controller::InputDevice("TouchscreenVirtualPad") {}
    private:
        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
        virtual void focusOutEvent() override;

        friend class TouchscreenVirtualPadDevice;

        controller::Input makeInput(TouchAxisChannel axis) const;
        controller::Input makeInput(TouchButtonChannel button) const;
    };

public:
    const std::shared_ptr<InputDevice>& getInputDevice() const { return _inputDevice; }

protected:

    enum TouchType {
        MOVE = 1,
        VIEW,
        JUMP
    };

    float _lastPinchScale;
    float _pinchScale;
    float _screenDPI;
    qreal _screenDPIProvided;
    glm::vec2 _screenDPIScale;

    bool _moveHasValidTouch;
    glm::vec2 _moveRefTouchPoint;
    glm::vec2 _moveCurrentTouchPoint;
    int _moveCurrentTouchId;

    bool _viewHasValidTouch;
    glm::vec2 _viewRefTouchPoint;
    glm::vec2 _viewCurrentTouchPoint;
    int _viewCurrentTouchId;

    bool _jumpHasValidTouch;
    int _jumpCurrentTouchId;

    std::map<int, TouchType> _unusedTouches;

    int _touchPointCount;
    int _screenWidthCenter;
    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };

    bool _fixedPosition;
    glm::vec2 _fixedCenterPosition;
    float _fixedRadius;
    float _fixedRadiusForCalc;
    int _extraBottomMargin {0};

    glm::vec2 _jumpButtonPosition;
    float _jumpButtonRadius;

    void moveTouchBegin(glm::vec2 touchPoint);
    void moveTouchUpdate(glm::vec2 touchPoint);
    void moveTouchEnd();
    bool moveTouchBeginIsValid(glm::vec2 touchPoint);

    void viewTouchBegin(glm::vec2 touchPoint);
    void viewTouchUpdate(glm::vec2 touchPoint);
    void viewTouchEnd();
    bool viewTouchBeginIsValid(glm::vec2 touchPoint);

    void jumpTouchBegin(glm::vec2 touchPoint);
    void jumpTouchUpdate(glm::vec2 touchPoint);
    void jumpTouchEnd();
    bool jumpTouchBeginIsValid(glm::vec2 touchPoint);

    void setupControlsPositions(VirtualPad::Manager& virtualPadManager, bool force = false);

    void processInputDeviceForMove(VirtualPad::Manager& virtualPadManager);
    glm::vec2 clippedPointInCircle(float radius, glm::vec2 origin, glm::vec2 touchPoint);

    void processUnusedTouches(std::map<int, TouchType> unusedTouchesInEvent);

    void processInputDeviceForView();
// just for debug
private:
    void debugPoints(const QTouchEvent* event, QString who);

};

#endif // hifi_TouchscreenVirtualPadDevice_h
