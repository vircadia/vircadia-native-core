//
//  Created by Bradley Austin Davis on 2015/10/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_controller_Actions_h
#define hifi_controller_Actions_h

#include <QtCore/QObject>
#include <QtCore/QVector>

#include "InputDevice.h"

namespace controller {

// Actions are the output channels of the Mapper, that's what the InputChannel map to
// For now the Actions are hardcoded, this is bad, but we will fix that in the near future
enum class Action {
    TRANSLATE_X = 0,
    TRANSLATE_Y,
    TRANSLATE_Z,
    ROTATE_X, PITCH = ROTATE_X,
    ROTATE_Y, YAW = ROTATE_Y,
    ROTATE_Z, ROLL = ROTATE_Z,

    STEP_YAW,
    // FIXME does this have a use case?
    STEP_PITCH,
    // FIXME does this have a use case?
    STEP_ROLL,

    STEP_TRANSLATE_X,
    STEP_TRANSLATE_Y,
    STEP_TRANSLATE_Z,

    TRANSLATE_CAMERA_Z,
    NUM_COMBINED_AXES,

    LEFT_HAND = NUM_COMBINED_AXES,
    RIGHT_HAND,

    LEFT_HAND_CLICK,
    RIGHT_HAND_CLICK,

    ACTION1,
    ACTION2,

    CONTEXT_MENU,
    TOGGLE_MUTE,

    SHIFT,

    // Biseced aliases for TRANSLATE_Z
    LONGITUDINAL_BACKWARD,
    LONGITUDINAL_FORWARD,

    // Biseced aliases for TRANSLATE_X
    LATERAL_LEFT,
    LATERAL_RIGHT,

    // Biseced aliases for TRANSLATE_Y
    VERTICAL_DOWN,
    VERTICAL_UP,

    // Biseced aliases for ROTATE_Y
    YAW_LEFT,
    YAW_RIGHT,

    // Biseced aliases for ROTATE_X
    PITCH_DOWN,
    PITCH_UP,

    // Biseced aliases for TRANSLATE_CAMERA_Z
    BOOM_IN,
    BOOM_OUT,

    NUM_ACTIONS,
};

template <typename T>
int toInt(T enumValue) { return static_cast<int>(enumValue); }

class ActionsDevice : public QObject, public InputDevice {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)

public:
    const QString& getName() const { return _name; }

    // Device functions
    virtual void buildDeviceProxy(DeviceProxy::Pointer proxy) override;
    virtual QString getDefaultMappingConfig() override;
    virtual void update(float deltaTime, bool jointsCaptured) override;
    virtual void focusOutEvent() override;

    ActionsDevice();
    virtual ~ActionsDevice();
};

}




#include "StandardControls.h"


#endif // hifi_StandardController_h
