//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UserInputMapper.h"
#include "StandardController.h"

#include "Logging.h"

const uint16_t UserInputMapper::ACTIONS_DEVICE = Input::INVALID_DEVICE - (uint16)1;
const uint16_t UserInputMapper::STANDARD_DEVICE = 0;

// Default contruct allocate the poutput size with the current hardcoded action channels
UserInputMapper::UserInputMapper() {
    registerStandardDevice();
    assignDefaulActionScales();
    createActionNames();
}

UserInputMapper::~UserInputMapper() {
}

int UserInputMapper::recordDeviceOfType(const QString& deviceName) {
    if (!_deviceCounts.contains(deviceName)) {
        _deviceCounts[deviceName] = 0;
    }
    _deviceCounts[deviceName] += 1;
    return _deviceCounts[deviceName];
}

bool UserInputMapper::registerDevice(uint16 deviceID, const DeviceProxy::Pointer& proxy) {
    int numberOfType = recordDeviceOfType(proxy->_name);
    
    if (numberOfType > 1) {
        proxy->_name += QString::number(numberOfType);
    }
    
    qCDebug(controllers) << "Registered input device <" << proxy->_name << "> deviceID = " << deviceID;
    _registeredDevices[deviceID] = proxy;
    return true;
    
}


bool UserInputMapper::registerStandardDevice(const DeviceProxy::Pointer& device) {
    device->_name = "Standard"; // Just to make sure
    _registeredDevices[getStandardDeviceID()] = device;
    return true;
}


UserInputMapper::DeviceProxy::Pointer UserInputMapper::getDeviceProxy(const Input& input) {
    auto device = _registeredDevices.find(input.getDevice());
    if (device != _registeredDevices.end()) {
        return (device->second);
    } else {
        return DeviceProxy::Pointer();
    }
}

QString UserInputMapper::getDeviceName(uint16 deviceID) { 
    if (_registeredDevices.find(deviceID) != _registeredDevices.end()) {
        return _registeredDevices[deviceID]->_name;
    }
    return QString("unknown");
}


void UserInputMapper::resetAllDeviceBindings() {
    for (auto device : _registeredDevices) {
        device.second->resetDeviceBindings();
    }
}

void UserInputMapper::resetDevice(uint16 deviceID) {
    auto device = _registeredDevices.find(deviceID);
    if (device != _registeredDevices.end()) {
        device->second->resetDeviceBindings();
    }
}

int UserInputMapper::findDevice(QString name) const {
    for (auto device : _registeredDevices) {
        if (device.second->_name.split(" (")[0] == name) {
            return device.first;
        } else if (device.second->_baseName == name) {
            return device.first;
        }
    }
    return Input::INVALID_DEVICE;
}

QVector<QString> UserInputMapper::getDeviceNames() {
    QVector<QString> result;
    for (auto device : _registeredDevices) {
        QString deviceName = device.second->_name.split(" (")[0];
        result << deviceName;
    }
    return result;
}

UserInputMapper::Input UserInputMapper::findDeviceInput(const QString& inputName) const {
    
    // Split the full input name as such: deviceName.inputName
    auto names = inputName.split('.');

    if (names.size() >= 2) {
        // Get the device name:
        auto deviceName = names[0];
        auto inputName = names[1];

        int deviceID = findDevice(deviceName);
        if (deviceID != Input::INVALID_DEVICE) {
            const auto& deviceProxy = _registeredDevices.at(deviceID);
            auto deviceInputs = deviceProxy->getAvailabeInputs();

            for (auto input : deviceInputs) {
                if (input.second == inputName) {
                    return input.first;
                }
            }

            qCDebug(controllers) << "Couldn\'t find InputChannel named <" << inputName << "> for device <" << deviceName << ">";

        } else if (deviceName == "Actions") {
            deviceID = ACTIONS_DEVICE;
            int actionNum = 0;
            for (auto action : _actionNames) {
                if (action == inputName) {
                    return Input(ACTIONS_DEVICE, actionNum, ChannelType::AXIS);
                }
                actionNum++;
            }

            qCDebug(controllers) << "Couldn\'t find ActionChannel named <" << inputName << "> among actions";

        } else {
            qCDebug(controllers) << "Couldn\'t find InputDevice named <" << deviceName << ">";
        }
    } else {
        qCDebug(controllers) << "Couldn\'t understand <" << inputName << "> as a valid inputDevice.inputName";
    }

    return Input::INVALID_INPUT;
}



bool UserInputMapper::addInputChannel(Action action, const Input& input, float scale) {
    return addInputChannel(action, input, Input(), scale);
}

bool UserInputMapper::addInputChannel(Action action, const Input& input, const Input& modifier, float scale) {
    // Check that the device is registered
    if (!getDeviceProxy(input)) {
        qDebug() << "UserInputMapper::addInputChannel: The input comes from a device #" << input.getDevice() << "is unknown. no inputChannel mapped.";
        return false;
    }
    
    auto inputChannel = InputChannel(input, modifier, action, scale);

    // Insert or replace the input to modifiers
    if (inputChannel.hasModifier()) {
        auto& modifiers = _inputToModifiersMap[input.getID()];
        modifiers.push_back(inputChannel._modifier);
        std::sort(modifiers.begin(), modifiers.end());
    }

    // Now update the action To Inputs side of things
    _actionToInputsMap.insert(ActionToInputsMap::value_type(action, inputChannel));

    return true;
}

int UserInputMapper::addInputChannels(const InputChannels& channels) {
    int nbAdded = 0;
    for (auto& channel : channels) {
        nbAdded += addInputChannel(channel._action, channel._input, channel._modifier, channel._scale);
    }
    return nbAdded;
}

bool UserInputMapper::removeInputChannel(InputChannel inputChannel) {
    // Remove from Input to Modifiers map
    if (inputChannel.hasModifier()) {
        _inputToModifiersMap.erase(inputChannel._input.getID());
    }
    
    // Remove from Action to Inputs map
    std::pair<ActionToInputsMap::iterator, ActionToInputsMap::iterator> ret;
    ret = _actionToInputsMap.equal_range(inputChannel._action);
    for (ActionToInputsMap::iterator it=ret.first; it!=ret.second; ++it) {
        if (it->second == inputChannel) {
            _actionToInputsMap.erase(it);
            return true;
        }
    }
    
    return false;
}

void UserInputMapper::removeAllInputChannels() {
    _inputToModifiersMap.clear();
    _actionToInputsMap.clear();
}

void UserInputMapper::removeAllInputChannelsForDevice(uint16 device) {
    QVector<InputChannel> channels = getAllInputsForDevice(device);
    for (auto& channel : channels) {
        removeInputChannel(channel);
    }
}

void UserInputMapper::removeDevice(int device) {
    removeAllInputChannelsForDevice((uint16) device);
    _registeredDevices.erase(device);
}

int UserInputMapper::getInputChannels(InputChannels& channels) const {
    for (auto& channel : _actionToInputsMap) {
        channels.push_back(channel.second);
    }

    return _actionToInputsMap.size();
}

QVector<UserInputMapper::InputChannel> UserInputMapper::getAllInputsForDevice(uint16 device) {
    InputChannels allChannels;
    getInputChannels(allChannels);
    
    QVector<InputChannel> channels;
    for (InputChannel inputChannel : allChannels) {
        if (inputChannel._input._device == device) {
            channels.push_back(inputChannel);
        }
    }
    
    return channels;
}

void fixBisectedAxis(float& full, float& negative, float& positive) {
    full = full + (negative * -1.0f) + positive;
    negative = full >= 0.0f ? 0.0f : full * -1.0f;
    positive = full <= 0.0f ? 0.0f : full;
}

void UserInputMapper::update(float deltaTime) {

    // Reset the axis state for next loop
    for (auto& channel : _actionStates) {
        channel = 0.0f;
    }
    
    for (auto& channel : _poseStates) {
        channel = PoseValue();
    }

    int currentTimestamp = 0;
    for (auto& channelInput : _actionToInputsMap) {
        auto& inputMapping = channelInput.second;
        auto& inputID = inputMapping._input;
        bool enabled = true;
        
        // Check if this input channel has modifiers and collect the possibilities
        auto modifiersIt = _inputToModifiersMap.find(inputID.getID());
        if (modifiersIt != _inputToModifiersMap.end()) {
            Modifiers validModifiers;
            bool isActiveModifier = false;
            for (auto& modifier : modifiersIt->second) {
                auto deviceProxy = getDeviceProxy(modifier);
                if (deviceProxy->getButton(modifier, currentTimestamp)) {
                    validModifiers.push_back(modifier);
                    isActiveModifier |= (modifier.getID() == inputMapping._modifier.getID());
                }
            }
            enabled = (validModifiers.empty() && !inputMapping.hasModifier()) || isActiveModifier;
        }

        // if enabled: default input or all modifiers on
        if (enabled) {
            auto deviceProxy = getDeviceProxy(inputID);
            switch (inputMapping._input.getType()) {
                case ChannelType::BUTTON: {
                    _actionStates[channelInput.first] += inputMapping._scale * float(deviceProxy->getButton(inputID, currentTimestamp));// * deltaTime; // weight the impulse by the deltaTime
                    break;
                }
                case ChannelType::AXIS: {
                    _actionStates[channelInput.first] += inputMapping._scale * deviceProxy->getAxis(inputID, currentTimestamp);
                    break;
                }
                case ChannelType::POSE: {
                    if (!_poseStates[channelInput.first].isValid()) {
                        _poseStates[channelInput.first] = deviceProxy->getPose(inputID, currentTimestamp);
                    }
                    break;
                }
                default: {
                    break; //silence please
                }
            }
        } else{
            // Channel input not enabled
            enabled = false;
        }
    }

    // Scale all the channel step with the scale
    for (auto i = 0; i < NUM_ACTIONS; i++) {
        if (_externalActionStates[i] != 0) {
            _actionStates[i] += _externalActionStates[i];
            _externalActionStates[i] = 0.0f;
        }
    }

    // merge the bisected and non-bisected axes for now
    fixBisectedAxis(_actionStates[TRANSLATE_X], _actionStates[LATERAL_LEFT], _actionStates[LATERAL_RIGHT]);
    fixBisectedAxis(_actionStates[TRANSLATE_Y], _actionStates[VERTICAL_DOWN], _actionStates[VERTICAL_UP]);
    fixBisectedAxis(_actionStates[TRANSLATE_Z], _actionStates[LONGITUDINAL_FORWARD], _actionStates[LONGITUDINAL_BACKWARD]);
    fixBisectedAxis(_actionStates[TRANSLATE_CAMERA_Z], _actionStates[BOOM_IN], _actionStates[BOOM_OUT]);
    fixBisectedAxis(_actionStates[ROTATE_Y], _actionStates[YAW_LEFT], _actionStates[YAW_RIGHT]);
    fixBisectedAxis(_actionStates[ROTATE_X], _actionStates[PITCH_UP], _actionStates[PITCH_DOWN]);


    static const float EPSILON = 0.01f;
    for (auto i = 0; i < NUM_ACTIONS; i++) {
        _actionStates[i] *= _actionScales[i];
        // Emit only on change, and emit when moving back to 0
        if (fabsf(_actionStates[i] - _lastActionStates[i]) > EPSILON) {
            _lastActionStates[i] = _actionStates[i];
            emit actionEvent(i, _actionStates[i]);
        }
        // TODO: emit signal for pose changes
    }
}

QVector<UserInputMapper::Action> UserInputMapper::getAllActions() const {
    QVector<Action> actions;
    for (auto i = 0; i < NUM_ACTIONS; i++) {
        actions.append(Action(i));
    }
    return actions;
}

QVector<UserInputMapper::InputChannel> UserInputMapper::getInputChannelsForAction(UserInputMapper::Action action) {
    QVector<InputChannel> inputChannels;
    std::pair <ActionToInputsMap::iterator, ActionToInputsMap::iterator> ret;
    ret = _actionToInputsMap.equal_range(action);
    for (ActionToInputsMap::iterator it=ret.first; it!=ret.second; ++it) {
        inputChannels.append(it->second);
    }
    return inputChannels;
}

int UserInputMapper::findAction(const QString& actionName) const {
    auto actions = getAllActions();
    for (auto action : actions) {
        if (getActionName(action) == actionName) {
            return action;
        }
    }
    // If the action isn't found, return -1
    return -1;
}

QVector<QString> UserInputMapper::getActionNames() const {
    QVector<QString> result;
    for (auto i = 0; i < NUM_ACTIONS; i++) {
        result << _actionNames[i];
    }
    return result;
}

void UserInputMapper::assignDefaulActionScales() {
    _actionScales[LONGITUDINAL_BACKWARD] = 1.0f; // 1m per unit
    _actionScales[LONGITUDINAL_FORWARD] = 1.0f; // 1m per unit
    _actionScales[LATERAL_LEFT] = 1.0f; // 1m per unit
    _actionScales[LATERAL_RIGHT] = 1.0f; // 1m per unit
    _actionScales[VERTICAL_DOWN] = 1.0f; // 1m per unit
    _actionScales[VERTICAL_UP] = 1.0f; // 1m per unit
    _actionScales[YAW_LEFT] = 1.0f; // 1 degree per unit
    _actionScales[YAW_RIGHT] = 1.0f; // 1 degree per unit
    _actionScales[PITCH_DOWN] = 1.0f; // 1 degree per unit
    _actionScales[PITCH_UP] = 1.0f; // 1 degree per unit
    _actionScales[BOOM_IN] = 0.5f; // .5m per unit
    _actionScales[BOOM_OUT] = 0.5f; // .5m per unit
    _actionScales[LEFT_HAND] = 1.0f; // default
    _actionScales[RIGHT_HAND] = 1.0f; // default
    _actionScales[LEFT_HAND_CLICK] = 1.0f; // on
    _actionScales[RIGHT_HAND_CLICK] = 1.0f; // on
    _actionScales[SHIFT] = 1.0f; // on
    _actionScales[ACTION1] = 1.0f; // default
    _actionScales[ACTION2] = 1.0f; // default
    _actionScales[TRANSLATE_X] = 1.0f; // default
    _actionScales[TRANSLATE_Y] = 1.0f; // default
    _actionScales[TRANSLATE_Z] = 1.0f; // default
    _actionScales[ROLL] = 1.0f; // default
    _actionScales[PITCH] = 1.0f; // default
    _actionScales[YAW] = 1.0f; // default
}

// This is only necessary as long as the actions are hardcoded
// Eventually you can just add the string when you add the action
void UserInputMapper::createActionNames() {
    _actionNames[LONGITUDINAL_BACKWARD] = "LONGITUDINAL_BACKWARD";
    _actionNames[LONGITUDINAL_FORWARD] = "LONGITUDINAL_FORWARD";
    _actionNames[LATERAL_LEFT] = "LATERAL_LEFT";
    _actionNames[LATERAL_RIGHT] = "LATERAL_RIGHT";
    _actionNames[VERTICAL_DOWN] = "VERTICAL_DOWN";
    _actionNames[VERTICAL_UP] = "VERTICAL_UP";
    _actionNames[YAW_LEFT] = "YAW_LEFT";
    _actionNames[YAW_RIGHT] = "YAW_RIGHT";
    _actionNames[PITCH_DOWN] = "PITCH_DOWN";
    _actionNames[PITCH_UP] = "PITCH_UP";
    _actionNames[BOOM_IN] = "BOOM_IN";
    _actionNames[BOOM_OUT] = "BOOM_OUT";
    _actionNames[LEFT_HAND] = "LEFT_HAND";
    _actionNames[RIGHT_HAND] = "RIGHT_HAND";
    _actionNames[LEFT_HAND_CLICK] = "LEFT_HAND_CLICK";
    _actionNames[RIGHT_HAND_CLICK] = "RIGHT_HAND_CLICK";
    _actionNames[SHIFT] = "SHIFT";
    _actionNames[ACTION1] = "ACTION1";
    _actionNames[ACTION2] = "ACTION2";
    _actionNames[CONTEXT_MENU] = "CONTEXT_MENU";
    _actionNames[TOGGLE_MUTE] = "TOGGLE_MUTE";
    _actionNames[TRANSLATE_X] = "TranslateX";
    _actionNames[TRANSLATE_Y] = "TranslateY";
    _actionNames[TRANSLATE_Z] = "TranslateZ";
    _actionNames[ROLL] = "Roll";
    _actionNames[PITCH] = "Pitch";
    _actionNames[YAW] = "Yaw";
}

void UserInputMapper::registerStandardDevice() {
    _standardController = std::make_shared<StandardController>();
    _standardController->registerToUserInputMapper(*this);
    _standardController->assignDefaultInputMapping(*this);
}

static int actionMetaTypeId = qRegisterMetaType<UserInputMapper::Action>();
static int inputMetaTypeId = qRegisterMetaType<UserInputMapper::Input>();
static int inputPairMetaTypeId = qRegisterMetaType<UserInputMapper::InputPair>();

QScriptValue inputToScriptValue(QScriptEngine* engine, const UserInputMapper::Input& input);
void inputFromScriptValue(const QScriptValue& object, UserInputMapper::Input& input);
QScriptValue actionToScriptValue(QScriptEngine* engine, const UserInputMapper::Action& action);
void actionFromScriptValue(const QScriptValue& object, UserInputMapper::Action& action);
QScriptValue inputPairToScriptValue(QScriptEngine* engine, const UserInputMapper::InputPair& inputPair);
void inputPairFromScriptValue(const QScriptValue& object, UserInputMapper::InputPair& inputPair);

QScriptValue inputToScriptValue(QScriptEngine* engine, const UserInputMapper::Input& input) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("device", input.getDevice());
    obj.setProperty("channel", input.getChannel());
    obj.setProperty("type", (unsigned short)input.getType());
    obj.setProperty("id", input.getID());
    return obj;
}

void inputFromScriptValue(const QScriptValue& object, UserInputMapper::Input& input) {
    input.setDevice(object.property("device").toUInt16());
    input.setChannel(object.property("channel").toUInt16());
    input.setType(object.property("type").toUInt16());
    input.setID(object.property("id").toInt32());
}

QScriptValue actionToScriptValue(QScriptEngine* engine, const UserInputMapper::Action& action) {
    QScriptValue obj = engine->newObject();
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    obj.setProperty("action", (int)action);
    obj.setProperty("actionName", userInputMapper->getActionName(action));
    return obj;
}

void actionFromScriptValue(const QScriptValue& object, UserInputMapper::Action& action) {
    action = UserInputMapper::Action(object.property("action").toVariant().toInt());
}

QScriptValue inputPairToScriptValue(QScriptEngine* engine, const UserInputMapper::InputPair& inputPair) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("input", inputToScriptValue(engine, inputPair.first));
    obj.setProperty("inputName", inputPair.second);
    return obj;
}

void inputPairFromScriptValue(const QScriptValue& object, UserInputMapper::InputPair& inputPair) {
    inputFromScriptValue(object.property("input"), inputPair.first);
    inputPair.second = QString(object.property("inputName").toVariant().toString());
}

void UserInputMapper::registerControllerTypes(QScriptEngine* engine) {
    qScriptRegisterSequenceMetaType<QVector<UserInputMapper::Action> >(engine);
    qScriptRegisterSequenceMetaType<QVector<UserInputMapper::InputPair> >(engine);
    qScriptRegisterMetaType(engine, actionToScriptValue, actionFromScriptValue);
    qScriptRegisterMetaType(engine, inputToScriptValue, inputFromScriptValue);
    qScriptRegisterMetaType(engine, inputPairToScriptValue, inputPairFromScriptValue);
}

UserInputMapper::Input UserInputMapper::makeStandardInput(controller::StandardButtonChannel button) {
    return Input(STANDARD_DEVICE, button, ChannelType::BUTTON);
}

UserInputMapper::Input UserInputMapper::makeStandardInput(controller::StandardAxisChannel axis) {
    return Input(STANDARD_DEVICE, axis, ChannelType::AXIS);
}

UserInputMapper::Input UserInputMapper::makeStandardInput(controller::StandardPoseChannel pose) {
    return Input(STANDARD_DEVICE, pose, ChannelType::POSE);
}
