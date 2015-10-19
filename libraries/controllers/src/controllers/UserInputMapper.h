//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_UserInputMapper_h
#define hifi_UserInputMapper_h

#include <glm/glm.hpp>

#include <unordered_set>
#include <functional>
#include <memory>
#include <DependencyManager.h>
#include <RegisteredMetaTypes.h>

#include "Pose.h"
#include "Input.h"
#include "DeviceProxy.h"

class StandardController;    
typedef std::shared_ptr<StandardController> StandardControllerPointer;

class UserInputMapper : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_ENUMS(Action)
public:
    ~UserInputMapper();

    using DeviceProxy = controller::DeviceProxy;
    using PoseValue = controller::Pose;
    using Input = controller::Input;
    using ChannelType = controller::ChannelType;

    typedef unsigned short uint16;
    typedef unsigned int uint32;

    static void registerControllerTypes(QScriptEngine* engine);

    static const uint16 ACTIONS_DEVICE;
    static const uint16 STANDARD_DEVICE;


    // Modifiers are just button inputID
    typedef std::vector< Input > Modifiers;
    typedef std::function<bool (const Input& input, int timestamp)> ButtonGetter;
    typedef std::function<float (const Input& input, int timestamp)> AxisGetter;
    typedef std::function<PoseValue (const Input& input, int timestamp)> PoseGetter;
    typedef QPair<Input, QString> InputPair;
    typedef std::function<QVector<InputPair> ()> AvailableInputGetter;
    typedef std::function<bool ()> ResetBindings;
    
    typedef QVector<InputPair> AvailableInput;

    // GetFreeDeviceID should be called before registering a device to use an ID not used by a different device.
    uint16 getFreeDeviceID() { return _nextFreeDeviceID++; }

    bool registerDevice(uint16 deviceID, const DeviceProxy::Pointer& device);
    bool registerStandardDevice(const DeviceProxy::Pointer& device);
    DeviceProxy::Pointer getDeviceProxy(const Input& input);
    QString getDeviceName(uint16 deviceID);
    QVector<InputPair> getAvailableInputs(uint16 deviceID) { return _registeredDevices[deviceID]->getAvailabeInputs(); }
    void resetAllDeviceBindings();
    void resetDevice(uint16 deviceID);
    int findDevice(QString name) const;
    QVector<QString> getDeviceNames();

    Input findDeviceInput(const QString& inputName) const;


    // Actions are the output channels of the Mapper, that's what the InputChannel map to
    // For now the Actions are hardcoded, this is bad, but we will fix that in the near future
    enum Action {
        TRANSLATE_X = 0,
        TRANSLATE_Y,
        TRANSLATE_Z,
        ROTATE_X, PITCH = ROTATE_X,
        ROTATE_Y, YAW = ROTATE_Y,
        ROTATE_Z, ROLL = ROTATE_Z,

        TRANSLATE_CAMERA_Z,

        LEFT_HAND,
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
    
    std::vector<QString> _actionNames = std::vector<QString>(NUM_ACTIONS);
    void createActionNames();

    QVector<Action> getAllActions() const;
    QString getActionName(Action action) const { return UserInputMapper::_actionNames[(int) action]; }
    float getActionState(Action action) const { return _actionStates[action]; }
    PoseValue getPoseState(Action action) const { return _poseStates[action]; }
    int findAction(const QString& actionName) const;
    QVector<QString> getActionNames() const;
    void assignDefaulActionScales();

    void setActionState(Action action, float value) { _externalActionStates[action] = value; }
    void deltaActionState(Action action, float delta) { _externalActionStates[action] += delta; }

    // Add input channel to the mapper and check that all the used channels are registered.
    // Return true if theinput channel is created correctly, false either
    bool addInputChannel(Action action, const Input& input, float scale = 1.0f);
    bool addInputChannel(Action action, const Input& input, const Input& modifer, float scale = 1.0f);

    // Under the hood, the input channels are organized in map sorted on the _output
    // The InputChannel class is just the full values describing the input channel in one object 
    class InputChannel {
    public:
        Input _input;
        Input _modifier = Input(); // make it invalid by default, meaning no modifier
        Action _action = LONGITUDINAL_BACKWARD;
        float _scale = 0.0f;
        
        Input getInput() const { return _input; }
        Input getModifier() const { return _modifier; }
        Action getAction() const { return _action; }
        float getScale() const { return _scale; }
        
        void setInput(Input input) { _input = input; }
        void setModifier(Input modifier) { _modifier = modifier; }
        void setAction(Action action) { _action = action; }
        void setScale(float scale) { _scale = scale; }

        InputChannel() {}
        InputChannel(const Input& input, const Input& modifier, Action action, float scale = 1.0f) :
            _input(input), _modifier(modifier), _action(action), _scale(scale) {}
        InputChannel(const InputChannel& src) : InputChannel(src._input, src._modifier, src._action, src._scale) {}
        InputChannel& operator = (const InputChannel& src) { _input = src._input; _modifier = src._modifier; _action = src._action; _scale = src._scale; return (*this); }
        bool operator ==(const InputChannel& right) const { return _input == right._input && _modifier == right._modifier && _action == right._action && _scale == right._scale; }
        bool hasModifier() { return _modifier.isValid(); }
    };
    typedef std::vector< InputChannel > InputChannels;

    // Add a bunch of input channels, return the true number of channels that successfully were added
    int addInputChannels(const InputChannels& channels);
    // Remove the first found instance of the input channel from the input mapper, true if found
    bool removeInputChannel(InputChannel channel);
    void removeAllInputChannels();
    void removeAllInputChannelsForDevice(uint16 device);
    void removeDevice(int device);
    //Grab all the input channels currently in use, return the number
    int getInputChannels(InputChannels& channels) const;
    QVector<InputChannel> getAllInputsForDevice(uint16 device);
    QVector<InputChannel> getInputChannelsForAction(UserInputMapper::Action action);
    std::multimap<Action, InputChannel> getActionToInputsMap() { return _actionToInputsMap; }

    // Update means go grab all the device input channels and update the output channel values
    void update(float deltaTime);
    
    void setSensorToWorldMat(glm::mat4 sensorToWorldMat) { _sensorToWorldMat = sensorToWorldMat; }
    glm::mat4 getSensorToWorldMat() { return _sensorToWorldMat; }
    
    UserInputMapper();

    typedef std::map<int, DeviceProxy::Pointer> DevicesMap;
    DevicesMap getDevices() { return _registeredDevices; }

    uint16 getStandardDeviceID() const { return STANDARD_DEVICE; }
    DeviceProxy::Pointer getStandardDevice() { return _registeredDevices[getStandardDeviceID()]; }

signals:
    void actionEvent(int action, float state);


protected:
    void registerStandardDevice();
    StandardControllerPointer _standardController;
        
    DevicesMap _registeredDevices;
    uint16 _nextFreeDeviceID = STANDARD_DEVICE + 1;

    typedef std::map<int, Modifiers> InputToMoModifiersMap;
    InputToMoModifiersMap _inputToModifiersMap;

    typedef std::multimap<Action, InputChannel> ActionToInputsMap;
    ActionToInputsMap _actionToInputsMap;
 
    std::vector<float> _actionStates = std::vector<float>(NUM_ACTIONS, 0.0f);
    std::vector<float> _externalActionStates = std::vector<float>(NUM_ACTIONS, 0.0f);
    std::vector<float> _actionScales = std::vector<float>(NUM_ACTIONS, 1.0f);
    std::vector<float> _lastActionStates = std::vector<float>(NUM_ACTIONS, 0.0f);
    std::vector<PoseValue> _poseStates = std::vector<PoseValue>(NUM_ACTIONS);

    glm::mat4 _sensorToWorldMat;

    int recordDeviceOfType(const QString& deviceName);
    QHash<const QString&, int> _deviceCounts;
};

Q_DECLARE_METATYPE(UserInputMapper::InputPair)
Q_DECLARE_METATYPE(UserInputMapper::PoseValue)
Q_DECLARE_METATYPE(QVector<UserInputMapper::InputPair>)
Q_DECLARE_METATYPE(UserInputMapper::Input)
Q_DECLARE_METATYPE(UserInputMapper::InputChannel)
Q_DECLARE_METATYPE(QVector<UserInputMapper::InputChannel>)
Q_DECLARE_METATYPE(UserInputMapper::Action)
Q_DECLARE_METATYPE(QVector<UserInputMapper::Action>)

#endif // hifi_UserInputMapper_h
