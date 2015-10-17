//
//  InputDevice.h
//  input-plugins/src/input-plugins
//
//  Created by Sam Gondelman on 7/15/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "UserInputMapper.h"
#include "StandardControls.h"

// Event types for each controller
const unsigned int CONTROLLER_0_EVENT = 1500U;
const unsigned int CONTROLLER_1_EVENT = 1501U;

// NOTE: If something inherits from both InputDevice and InputPlugin, InputPlugin must go first.
// e.g. class Example : public InputPlugin, public InputDevice
// instead of class Example : public InputDevice, public InputPlugin
class InputDevice {
public:
    InputDevice(const QString& name) : _name(name) {}

    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap;
    typedef std::map<int, UserInputMapper::PoseValue> PoseStateMap;

    // Get current state for each channel
    float getButton(int channel) const;
    float getAxis(int channel) const;
    UserInputMapper::PoseValue getPose(int channel) const;

    virtual void registerToUserInputMapper(UserInputMapper& mapper) = 0;
    virtual void assignDefaultInputMapping(UserInputMapper& mapper) {};

    // Update call MUST be called once per simulation loop
    // It takes care of updating the action states and deltas
    virtual void update(float deltaTime, bool jointsCaptured) = 0;

    virtual void focusOutEvent() = 0;

    int getDeviceID() { return _deviceID; }

    static float getCursorPixelRangeMult();
    static float getReticleMoveSpeed() { return reticleMoveSpeed; }
    static void setReticleMoveSpeed(float sixenseReticleMoveSpeed) { reticleMoveSpeed = sixenseReticleMoveSpeed; }

    static bool getLowVelocityFilter() { return _lowVelocityFilter; };

    UserInputMapper::Input makeInput(controller::StandardButtonChannel button);
    UserInputMapper::Input makeInput(controller::StandardAxisChannel axis);
    UserInputMapper::Input makeInput(controller::StandardPoseChannel pose);
    UserInputMapper::InputPair makePair(controller::StandardButtonChannel button, const QString& name);
    UserInputMapper::InputPair makePair(controller::StandardAxisChannel button, const QString& name);
    UserInputMapper::InputPair makePair(controller::StandardPoseChannel button, const QString& name);
public slots:
    static void setLowVelocityFilter(bool newLowVelocityFilter) { _lowVelocityFilter = newLowVelocityFilter; };

protected:

    int _deviceID = 0;

    QString _name;

    ButtonPressedMap _buttonPressedMap;
    AxisStateMap _axisStateMap;
    PoseStateMap _poseStateMap;

    static bool _lowVelocityFilter;

private:
    static float reticleMoveSpeed;
};