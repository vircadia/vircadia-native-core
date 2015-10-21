//
//  StandardController.h
//  input-plugins/src/input-plugins
//
//  Created by Brad Hefta-Gaub on 2015-10-11.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_StandardController_h
#define hifi_StandardController_h

#include <qobject.h>
#include <qvector.h>

#include "InputDevice.h"

typedef std::shared_ptr<StandardController> StandardControllerPointer;

class StandardController : public QObject, public InputDevice {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)

public:
    enum StandardControllerAxisChannel {
        LEFT_AXIS_X_POS = 0,
        LEFT_AXIS_X_NEG,
        LEFT_AXIS_Y_POS,
        LEFT_AXIS_Y_NEG,
        RIGHT_AXIS_X_POS,
        RIGHT_AXIS_X_NEG,
        RIGHT_AXIS_Y_POS,
        RIGHT_AXIS_Y_NEG,
        RIGHT_SHOULDER,
        LEFT_SHOULDER,
    };
    enum StandardControllerButtonChannel {
        STANDARD_CONTROLLER_BUTTON_A = 0,
        STANDARD_CONTROLLER_BUTTON_B,
        STANDARD_CONTROLLER_BUTTON_X,
        STANDARD_CONTROLLER_BUTTON_Y,

        STANDARD_CONTROLLER_BUTTON_DPAD_UP,
        STANDARD_CONTROLLER_BUTTON_DPAD_DOWN,
        STANDARD_CONTROLLER_BUTTON_DPAD_LEFT,
        STANDARD_CONTROLLER_BUTTON_DPAD_RIGHT,

        STANDARD_CONTROLLER_BUTTON_LEFTSHOULDER,
        STANDARD_CONTROLLER_BUTTON_RIGHTSHOULDER,
    };

    enum StandardControllerPoseChannel {
        LEFT_HAND = 0,
        RIGHT_HAND,
    };

    const QString& getName() const { return _name; }

    // Device functions
    virtual void registerToUserInputMapper(UserInputMapper& mapper) override;
    virtual void assignDefaultInputMapping(UserInputMapper& mapper) override;
    virtual void update(float deltaTime, bool jointsCaptured) override;
    virtual void focusOutEvent() override;
    
    StandardController() : InputDevice("Standard") {}
    ~StandardController();
    
    UserInputMapper::Input makeInput(StandardController::StandardControllerButtonChannel button);
    UserInputMapper::Input makeInput(StandardController::StandardControllerAxisChannel axis);
    UserInputMapper::Input makeInput(StandardController::StandardControllerPoseChannel pose);

private:
};

#endif // hifi_StandardController_h
