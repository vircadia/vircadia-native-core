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
            makePosePair(Action::RIGHT_ARM, "RightArm"),
            makePosePair(Action::LEFT_ARM, "LeftArm"),
            makePosePair(Action::LEFT_FOOT, "LeftFoot"),
            makePosePair(Action::RIGHT_FOOT, "RightFoot"),
            makePosePair(Action::HIPS, "Hips"),
            makePosePair(Action::SPINE2, "Spine2"),
            makePosePair(Action::HEAD, "Head"),

            makePosePair(Action::LEFT_HAND_THUMB1, "LeftHandThumb1"),
            makePosePair(Action::LEFT_HAND_THUMB2, "LeftHandThumb2"),
            makePosePair(Action::LEFT_HAND_THUMB3, "LeftHandThumb3"),
            makePosePair(Action::LEFT_HAND_THUMB4, "LeftHandThumb4"),
            makePosePair(Action::LEFT_HAND_INDEX1, "LeftHandIndex1"),
            makePosePair(Action::LEFT_HAND_INDEX2, "LeftHandIndex2"),
            makePosePair(Action::LEFT_HAND_INDEX3, "LeftHandIndex3"),
            makePosePair(Action::LEFT_HAND_INDEX4, "LeftHandIndex4"),
            makePosePair(Action::LEFT_HAND_MIDDLE1, "LeftHandMiddle1"),
            makePosePair(Action::LEFT_HAND_MIDDLE2, "LeftHandMiddle2"),
            makePosePair(Action::LEFT_HAND_MIDDLE3, "LeftHandMiddle3"),
            makePosePair(Action::LEFT_HAND_MIDDLE4, "LeftHandMiddle4"),
            makePosePair(Action::LEFT_HAND_RING1, "LeftHandRing1"),
            makePosePair(Action::LEFT_HAND_RING2, "LeftHandRing2"),
            makePosePair(Action::LEFT_HAND_RING3, "LeftHandRing3"),
            makePosePair(Action::LEFT_HAND_RING4, "LeftHandRing4"),
            makePosePair(Action::LEFT_HAND_PINKY1, "LeftHandPinky1"),
            makePosePair(Action::LEFT_HAND_PINKY2, "LeftHandPinky2"),
            makePosePair(Action::LEFT_HAND_PINKY3, "LeftHandPinky3"),
            makePosePair(Action::LEFT_HAND_PINKY4, "LeftHandPinky4"),

            makePosePair(Action::RIGHT_HAND_THUMB1, "RightHandThumb1"),
            makePosePair(Action::RIGHT_HAND_THUMB2, "RightHandThumb2"),
            makePosePair(Action::RIGHT_HAND_THUMB3, "RightHandThumb3"),
            makePosePair(Action::RIGHT_HAND_THUMB4, "RightHandThumb4"),
            makePosePair(Action::RIGHT_HAND_INDEX1, "RightHandIndex1"),
            makePosePair(Action::RIGHT_HAND_INDEX2, "RightHandIndex2"),
            makePosePair(Action::RIGHT_HAND_INDEX3, "RightHandIndex3"),
            makePosePair(Action::RIGHT_HAND_INDEX4, "RightHandIndex4"),
            makePosePair(Action::RIGHT_HAND_MIDDLE1, "RightHandMiddle1"),
            makePosePair(Action::RIGHT_HAND_MIDDLE2, "RightHandMiddle2"),
            makePosePair(Action::RIGHT_HAND_MIDDLE3, "RightHandMiddle3"),
            makePosePair(Action::RIGHT_HAND_MIDDLE4, "RightHandMiddle4"),
            makePosePair(Action::RIGHT_HAND_RING1, "RightHandRing1"),
            makePosePair(Action::RIGHT_HAND_RING2, "RightHandRing2"),
            makePosePair(Action::RIGHT_HAND_RING3, "RightHandRing3"),
            makePosePair(Action::RIGHT_HAND_RING4, "RightHandRing4"),
            makePosePair(Action::RIGHT_HAND_PINKY1, "RightHandPinky1"),
            makePosePair(Action::RIGHT_HAND_PINKY2, "RightHandPinky2"),
            makePosePair(Action::RIGHT_HAND_PINKY3, "RightHandPinky3"),
            makePosePair(Action::RIGHT_HAND_PINKY4, "RightHandPinky4"),

            makePosePair(Action::TRACKED_OBJECT_00, "TrackedObject00"),
            makePosePair(Action::TRACKED_OBJECT_01, "TrackedObject01"),
            makePosePair(Action::TRACKED_OBJECT_02, "TrackedObject02"),
            makePosePair(Action::TRACKED_OBJECT_03, "TrackedObject03"),
            makePosePair(Action::TRACKED_OBJECT_04, "TrackedObject04"),
            makePosePair(Action::TRACKED_OBJECT_05, "TrackedObject05"),
            makePosePair(Action::TRACKED_OBJECT_06, "TrackedObject06"),
            makePosePair(Action::TRACKED_OBJECT_07, "TrackedObject07"),
            makePosePair(Action::TRACKED_OBJECT_08, "TrackedObject08"),
            makePosePair(Action::TRACKED_OBJECT_09, "TrackedObject09"),
            makePosePair(Action::TRACKED_OBJECT_10, "TrackedObject10"),
            makePosePair(Action::TRACKED_OBJECT_11, "TrackedObject11"),
            makePosePair(Action::TRACKED_OBJECT_12, "TrackedObject12"),
            makePosePair(Action::TRACKED_OBJECT_13, "TrackedObject13"),
            makePosePair(Action::TRACKED_OBJECT_14, "TrackedObject14"),
            makePosePair(Action::TRACKED_OBJECT_15, "TrackedObject15"),

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
