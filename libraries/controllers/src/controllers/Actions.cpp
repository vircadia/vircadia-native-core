//
//  Created by Bradley Austin Davis on 2015/10/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Actions.h"

#include "UserInputMapper.h"

namespace controller {

    // Device functions
    void ActionsDevice::buildDeviceProxy(DeviceProxy::Pointer proxy) {
        proxy->_name = _name;
        proxy->getButton = [this](const Input& input, int timestamp) -> bool { return false; };
        proxy->getAxis = [this](const Input& input, int timestamp) -> float { return 0; };
        proxy->getAvailabeInputs = [this]() -> QVector<Input::NamedPair> {
            QVector<Input::NamedPair> availableInputs{
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::LONGITUDINAL_BACKWARD), ChannelType::AXIS), "LONGITUDINAL_BACKWARD"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::LONGITUDINAL_FORWARD), ChannelType::AXIS), "LONGITUDINAL_FORWARD"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::LATERAL_LEFT), ChannelType::AXIS), "LATERAL_LEFT"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::LATERAL_RIGHT), ChannelType::AXIS), "LATERAL_RIGHT"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::VERTICAL_DOWN), ChannelType::AXIS), "VERTICAL_DOWN"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::VERTICAL_UP), ChannelType::AXIS), "VERTICAL_UP"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::YAW_LEFT), ChannelType::AXIS), "YAW_LEFT"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::YAW_RIGHT), ChannelType::AXIS), "YAW_RIGHT"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::PITCH_DOWN), ChannelType::AXIS), "PITCH_DOWN"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::PITCH_UP), ChannelType::AXIS), "PITCH_UP"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::BOOM_IN), ChannelType::AXIS), "BOOM_IN"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::BOOM_OUT), ChannelType::AXIS), "BOOM_OUT"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::LEFT_HAND), ChannelType::POSE), "LEFT_HAND"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::RIGHT_HAND), ChannelType::POSE), "RIGHT_HAND"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::LEFT_HAND_CLICK), ChannelType::BUTTON), "LEFT_HAND_CLICK"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::RIGHT_HAND_CLICK), ChannelType::BUTTON), "RIGHT_HAND_CLICK"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::SHIFT), ChannelType::BUTTON), "SHIFT"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::ACTION1), ChannelType::BUTTON), "ACTION1"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::ACTION2), ChannelType::BUTTON), "ACTION2"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::CONTEXT_MENU), ChannelType::BUTTON), "CONTEXT_MENU"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::TOGGLE_MUTE), ChannelType::AXIS), "TOGGLE_MUTE"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::TRANSLATE_X), ChannelType::AXIS), "TranslateX"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::TRANSLATE_Y), ChannelType::AXIS), "TranslateY"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::TRANSLATE_Z), ChannelType::AXIS), "TranslateZ"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::ROLL), ChannelType::AXIS), "Roll"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::PITCH), ChannelType::AXIS), "Pitch"),
                Input::NamedPair(Input(UserInputMapper::ACTIONS_DEVICE, toInt(Action::YAW), ChannelType::AXIS), "Yaw")
            };
            return availableInputs;
        };

    }

    QString ActionsDevice::getDefaultMappingConfig() {
        return QString();
    }

    void ActionsDevice::update(float deltaTime, bool jointsCaptured) {
    }

    void ActionsDevice::focusOutEvent() {
    }

    ActionsDevice::ActionsDevice() : InputDevice("Actions") {
        _deviceID = UserInputMapper::ACTIONS_DEVICE;
    }

    ActionsDevice::~ActionsDevice() {}

}
