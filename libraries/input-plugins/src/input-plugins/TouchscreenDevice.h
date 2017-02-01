//
//  TouchscreenDevice.h
//  input-plugins/src/input-plugins
//
//  Created by Triplelexx on 1/31/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TouchscreenDevice_h
#define hifi_TouchscreenDevice_h

#include <controllers/InputDevice.h>
#include "InputPlugin.h"
#include <QtGui/qtouchdevice.h>

class QTouchEvent;
class QGestureEvent;

class TouchscreenDevice : public InputPlugin {
    Q_OBJECT
public:

    enum TouchAxisChannel {
        TOUCH_AXIS_X_POS = 0,
        TOUCH_AXIS_X_NEG,
        TOUCH_AXIS_Y_POS,
        TOUCH_AXIS_Y_NEG,
    };

	enum TouchGestureAxisChannel {
        TOUCH_GESTURE_PINCH_POS = TOUCH_AXIS_Y_NEG + 1,
        TOUCH_GESTURE_PINCH_NEG,
    };

    // Plugin functions
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

protected:

    class InputDevice : public controller::InputDevice {
    public:
        InputDevice() : controller::InputDevice("Touchscreen") {}
    private:
        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
        virtual void focusOutEvent() override;

        controller::Input makeInput(TouchAxisChannel axis) const;
        controller::Input makeInput(TouchGestureAxisChannel gesture) const;

        friend class TouchscreenDevice;
    };

public:
    const std::shared_ptr<InputDevice>& getInputDevice() const { return _inputDevice; }

protected:
    qreal _lastPinchScale;
    qreal _pinchScale;
    qreal _screenDPI;
    glm::vec2 _screenDPIScale;
    glm::vec2 _firstTouchVec;
    glm::vec2 _currentTouchVec;
    int _touchPointCount;
    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };
};

#endif // hifi_TouchscreenDevice_h
