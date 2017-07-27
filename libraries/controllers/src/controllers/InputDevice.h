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

#include <memory>
#include <map>
#include <unordered_set>

#include <QtCore/QString>

#include "Pose.h"
#include "Input.h"
#include "StandardControls.h"
#include "DeviceProxy.h"


// Event types for each controller
const unsigned int CONTROLLER_0_EVENT = 1500U;
const unsigned int CONTROLLER_1_EVENT = 1501U;

namespace controller {

class Endpoint;
using EndpointPointer = std::shared_ptr<Endpoint>;

enum Hand {
    LEFT = 0,
    RIGHT,
    BOTH
};

// NOTE: If something inherits from both InputDevice and InputPlugin, InputPlugin must go first.
// e.g. class Example : public InputPlugin, public InputDevice
// instead of class Example : public InputDevice, public InputPlugin
class InputDevice {
public:
    InputDevice(const QString& name) : _name(name) {}

    using Pointer = std::shared_ptr<InputDevice>;

    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap;
    typedef std::map<int, Pose> PoseStateMap;

    // Get current state for each channel
    float getButton(int channel) const;
    float getAxis(int channel) const;
    Pose getPose(int channel) const;

    float getValue(const Input& input) const;
    float getValue(ChannelType channelType, uint16_t channel) const;
    Pose getPoseValue(uint16_t channel) const;

    const QString& getName() const { return _name; }

    // By default, Input Devices do not support haptics
    virtual bool triggerHapticPulse(float strength, float duration, controller::Hand hand) { return false; }

    // Update call MUST be called once per simulation loop
    // It takes care of updating the action states and deltas
    virtual void update(float deltaTime, const InputCalibrationData& inputCalibrationData) {};

    virtual void focusOutEvent() {};

    int getDeviceID() { return _deviceID; }
    void setDeviceID(int deviceID) { _deviceID = deviceID; }

    Input makeInput(StandardButtonChannel button) const;
    Input makeInput(StandardAxisChannel axis) const;
    Input makeInput(StandardPoseChannel pose) const;
    Input::NamedPair makePair(StandardButtonChannel button, const QString& name) const;
    Input::NamedPair makePair(StandardAxisChannel button, const QString& name) const;
    Input::NamedPair makePair(StandardPoseChannel button, const QString& name) const;
    
protected:
    friend class UserInputMapper;

    virtual Input::NamedVector getAvailableInputs() const = 0;
    virtual QStringList getDefaultMappingConfigs() const { return QStringList() << getDefaultMappingConfig(); }
    virtual QString getDefaultMappingConfig() const { return QString(); }
    virtual EndpointPointer createEndpoint(const Input& input) const;

    uint16_t _deviceID { Input::INVALID_DEVICE };

    const QString _name;

    ButtonPressedMap _buttonPressedMap;
    AxisStateMap _axisStateMap;
    PoseStateMap _poseStateMap;
};

}
