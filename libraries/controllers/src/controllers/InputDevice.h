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

#include "AxisValue.h"
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

/*@jsdoc
 * <p>Some controller actions may be associated with one or both hands:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code></td><td>Left hand.</td></tr>
 *     <tr><td><code>1</code></td><td>Right hand.</td></tr>
 *     <tr><td><code>2</code></td><td>Both hands.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} Controller.Hand
 */
enum Hand {
    LEFT = 0,
    RIGHT,
    BOTH
};

/*@jsdoc
 * <p>The <code>Controller.Hardware</code> object has properties representing standard and hardware-specific controller and 
 * computer outputs, plus predefined actions on Interface and the user's avatar. <em>Read-only.</em></p>
 * <p>The outputs can be mapped to actions or functions in a {@link RouteObject} mapping. Additionally, hardware-specific 
 * controller outputs can be mapped to standard controller outputs. 
 * <p>Controllers typically implement a subset of the {@link Controller.Standard} controls, plus they may implement some extras. 
 * Some common controllers are included in the table. You can see the outputs provided by these and others by 
 * viewing their {@link Controller.MappingJSON|MappingJSON} files at 
 * <a href="https://github.com/highfidelity/hifi/tree/master/interface/resources/controllers">
 * https://github.com/highfidelity/hifi/tree/master/interface/resources/controllers</a>.</p>
 *
 * <table>
 *   <thead>
 *     <tr><th>Property</th><th>Type</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>Controller.Hardware.Actions</code></td><td>object</td><td>Synonym for {@link Controller.Actions}.</td></tr>
 *     <tr><td><code>Controller.Hardware.Application</code></td><td>object</td><td>Interface state outputs. See 
 *       {@link Controller.Hardware-Application}.</td></tr>
 *     <tr><td><code>Controller.Hardware.Keyboard</code></td><td>object</td><td>Keyboard, mouse, and touch pad outputs. See
 *       {@link Controller.Hardware-Keyboard}.</td></tr>
 *     <tr><td><code>Controller.Hardware.OculusTouch</code></td><td>object</td><td>Oculus Rift HMD outputs. See
 *       {@link Controller.Hardware-OculusTouch}.</td></tr>
 *     <tr><td><code>Controller.Hardware.Vive</code></td><td>object</td><td>Vive HMD outputs. See
 *       {@link Controller.Hardware-Vive}.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {object} Controller.Hardware
 * @example <caption>List all the currently available <code>Controller.Hardware</code> properties.</caption>
 * function printProperties(string, item) {
 *     print(string);
 *     for (var key in item) {
 *         if (item.hasOwnProperty(key)) {
 *             printProperties(string + "." + key, item[key]);
 *         }
 *     }
 * }
 *
 * printProperties("Controller.Hardware", Controller.Hardware);
 */
// NOTE: If something inherits from both InputDevice and InputPlugin, InputPlugin must go first.
// e.g. class Example : public InputPlugin, public InputDevice
// instead of class Example : public InputDevice, public InputPlugin
class InputDevice {
public:
    InputDevice(const QString& name) : _name(name) {}
    virtual ~InputDevice() = default;

    using Pointer = std::shared_ptr<InputDevice>;

    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, AxisValue> AxisStateMap;
    typedef std::map<int, Pose> PoseStateMap;

    // Get current state for each channel
    float getButton(int channel) const;
    AxisValue getAxis(int channel) const;
    Pose getPose(int channel) const;

    AxisValue getValue(const Input& input) const;
    AxisValue getValue(ChannelType channelType, uint16_t channel) const;
    Pose getPoseValue(uint16_t channel) const;

    const QString& getName() const { return _name; }

    // By default, Input Devices do not support haptics
    virtual bool triggerHapticPulse(float strength, float duration, uint16_t index) { return false; }

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
