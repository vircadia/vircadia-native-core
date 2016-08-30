//
//  Created by Bradley Austin Davis on 2015/10/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Actions.h"

#include "impl/endpoints/ActionEndpoint.h"

namespace controller {

    Input::NamedPair makePair(ChannelType type, Action action, const QString& name) {
        auto input = Input(UserInputMapper::ACTIONS_DEVICE, toInt(action), type);
        return Input::NamedPair(input, name);
    }

    Input::NamedPair makeAxisPair(Action action, const QString& name) {
        return makePair(ChannelType::AXIS, action, name);
    }

    Input::NamedPair makeButtonPair(Action action, const QString& name) {
        return makePair(ChannelType::BUTTON, action, name);
    }

    Input::NamedPair makePosePair(Action action, const QString& name) {
        return makePair(ChannelType::POSE, action, name);
    }

    EndpointPointer ActionsDevice::createEndpoint(const Input& input) const {
        return std::make_shared<ActionEndpoint>(input);
    }

    // Device functions
    Input::NamedVector ActionsDevice::getAvailableInputs() const {
        static Input::NamedVector availableInputs {
            makeAxisPair(Action::TRANSLATE_X, "TranslateX"),
            makeAxisPair(Action::TRANSLATE_Y, "TranslateY"),
            makeAxisPair(Action::TRANSLATE_Z, "TranslateZ"),
            makeAxisPair(Action::ROLL, "Roll"),
            makeAxisPair(Action::PITCH, "Pitch"),
            makeAxisPair(Action::YAW, "Yaw"),
            makeAxisPair(Action::STEP_YAW, "StepYaw"),
            makeAxisPair(Action::STEP_PITCH, "StepPitch"),
            makeAxisPair(Action::STEP_ROLL, "StepRoll"),
            makeAxisPair(Action::STEP_TRANSLATE_X, "StepTranslateX"),
            makeAxisPair(Action::STEP_TRANSLATE_X, "StepTranslateY"),
            makeAxisPair(Action::STEP_TRANSLATE_X, "StepTranslateZ"),

            makePosePair(Action::LEFT_HAND, "LeftHand"),
            makePosePair(Action::RIGHT_HAND, "RightHand"),

            makeButtonPair(Action::LEFT_HAND_CLICK, "LeftHandClick"),
            makeButtonPair(Action::RIGHT_HAND_CLICK, "RightHandClick"),

            makeButtonPair(Action::SHIFT, "Shift"),
            makeButtonPair(Action::ACTION1, "PrimaryAction"),
            makeButtonPair(Action::ACTION2, "SecondaryAction"),
            makeButtonPair(Action::CONTEXT_MENU, "ContextMenu"),
            makeButtonPair(Action::TOGGLE_MUTE, "ToggleMute"),
            makeButtonPair(Action::CYCLE_CAMERA, "CycleCamera"),
            makeButtonPair(Action::TOGGLE_OVERLAY, "ToggleOverlay"),

            makeAxisPair(Action::RETICLE_CLICK, "ReticleClick"),
            makeAxisPair(Action::RETICLE_X, "ReticleX"),
            makeAxisPair(Action::RETICLE_Y, "ReticleY"),
            makeAxisPair(Action::RETICLE_LEFT, "ReticleLeft"),
            makeAxisPair(Action::RETICLE_RIGHT, "ReticleRight"),
            makeAxisPair(Action::RETICLE_UP, "ReticleUp"),
            makeAxisPair(Action::RETICLE_DOWN, "ReticleDown"),

            makeAxisPair(Action::UI_NAV_LATERAL, "UiNavLateral"),
            makeAxisPair(Action::UI_NAV_VERTICAL, "UiNavVertical"),
            makeAxisPair(Action::UI_NAV_GROUP, "UiNavGroup"),
            makeAxisPair(Action::UI_NAV_SELECT, "UiNavSelect"),
            makeAxisPair(Action::UI_NAV_BACK, "UiNavBack"),

            // Aliases and bisected versions
            makeAxisPair(Action::LONGITUDINAL_BACKWARD, "Backward"),
            makeAxisPair(Action::LONGITUDINAL_FORWARD, "Forward"),
            makeAxisPair(Action::LATERAL_LEFT, "StrafeLeft"),
            makeAxisPair(Action::LATERAL_RIGHT, "StrafeRight"),
            makeAxisPair(Action::VERTICAL_DOWN, "Down"),
            makeAxisPair(Action::VERTICAL_UP, "Up"),
            makeAxisPair(Action::YAW_LEFT, "YawLeft"),
            makeAxisPair(Action::YAW_RIGHT, "YawRight"),
            makeAxisPair(Action::PITCH_DOWN, "PitchDown"),
            makeAxisPair(Action::PITCH_UP, "PitchUp"),
            makeAxisPair(Action::BOOM_IN, "BoomIn"),
            makeAxisPair(Action::BOOM_OUT, "BoomOut"),

            // Deprecated aliases
            // FIXME remove after we port all scripts
            makeAxisPair(Action::LONGITUDINAL_BACKWARD, "LONGITUDINAL_BACKWARD"),
            makeAxisPair(Action::LONGITUDINAL_FORWARD, "LONGITUDINAL_FORWARD"),
            makeAxisPair(Action::LATERAL_LEFT, "LATERAL_LEFT"),
            makeAxisPair(Action::LATERAL_RIGHT, "LATERAL_RIGHT"),
            makeAxisPair(Action::VERTICAL_DOWN, "VERTICAL_DOWN"),
            makeAxisPair(Action::VERTICAL_UP, "VERTICAL_UP"),
            makeAxisPair(Action::YAW_LEFT, "YAW_LEFT"),
            makeAxisPair(Action::YAW_RIGHT, "YAW_RIGHT"),
            makeAxisPair(Action::PITCH_DOWN, "PITCH_DOWN"),
            makeAxisPair(Action::PITCH_UP, "PITCH_UP"),
            makeAxisPair(Action::BOOM_IN, "BOOM_IN"),
            makeAxisPair(Action::BOOM_OUT, "BOOM_OUT"),

            makePosePair(Action::LEFT_HAND, "LEFT_HAND"),
            makePosePair(Action::RIGHT_HAND, "RIGHT_HAND"),

            makeButtonPair(Action::LEFT_HAND_CLICK, "LEFT_HAND_CLICK"),
            makeButtonPair(Action::RIGHT_HAND_CLICK, "RIGHT_HAND_CLICK"),

            makeButtonPair(Action::SHIFT, "SHIFT"),
            makeButtonPair(Action::ACTION1, "ACTION1"),
            makeButtonPair(Action::ACTION2, "ACTION2"),
            makeButtonPair(Action::CONTEXT_MENU, "CONTEXT_MENU"),
            makeButtonPair(Action::TOGGLE_MUTE, "TOGGLE_MUTE"),
        };
        return availableInputs;
    }

    ActionsDevice::ActionsDevice() : InputDevice("Actions") {
        _deviceID = UserInputMapper::ACTIONS_DEVICE;
    }

}
