//
//  StandardController.cpp
//  input-plugins/src/input-plugins
//
//  Created by Brad Hefta-Gaub on 2015-10-11.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StandardController.h"

#include <PathUtils.h>

#include "UserInputMapper.h"
#include "impl/endpoints/StandardEndpoint.h"

namespace controller {

StandardController::StandardController() : InputDevice("Standard") { 
    _deviceID = UserInputMapper::STANDARD_DEVICE; 
}

void StandardController::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

Input::NamedVector StandardController::getAvailableInputs() const {
    static Input::NamedVector availableInputs {
        // Buttons
        makePair(A, "A"),
        makePair(B, "B"),
        makePair(X, "X"),
        makePair(Y, "Y"),

        // DPad
        makePair(DU, "DU"),
        makePair(DD, "DD"),
        makePair(DL, "DL"),
        makePair(DR, "DR"),

        // Bumpers
        makePair(LB, "LB"),
        makePair(RB, "RB"),

        // Stick press
        makePair(LS, "LS"),
        makePair(RS, "RS"),

        makePair(LS_TOUCH, "LSTouch"),
        makePair(RS_TOUCH, "RSTouch"),

        // Center buttons
        makePair(START, "Start"),
        makePair(BACK, "Back"),

        // Analog sticks
        makePair(LY, "LY"),
        makePair(LX, "LX"),
        makePair(RY, "RY"),
        makePair(RX, "RX"),

        // Triggers
        makePair(LT, "LT"),
        makePair(RT, "RT"),
        makePair(LT_CLICK, "LTClick"),
        makePair(RT_CLICK, "RTClick"),

        // Finger abstractions
        makePair(LEFT_PRIMARY_THUMB, "LeftPrimaryThumb"),
        makePair(LEFT_SECONDARY_THUMB, "LeftSecondaryThumb"),
        makePair(LEFT_THUMB_UP, "LeftThumbUp"),
        makePair(RIGHT_PRIMARY_THUMB, "RightPrimaryThumb"),
        makePair(RIGHT_SECONDARY_THUMB, "RightSecondaryThumb"),
        makePair(RIGHT_THUMB_UP, "RightThumbUp"),

        makePair(LEFT_PRIMARY_THUMB_TOUCH, "LeftPrimaryThumbTouch"),
        makePair(LEFT_SECONDARY_THUMB_TOUCH, "LeftSecondaryThumbTouch"),
        makePair(RIGHT_PRIMARY_THUMB_TOUCH, "RightPrimaryThumbTouch"),
        makePair(RIGHT_SECONDARY_THUMB_TOUCH, "RightSecondaryThumbTouch"),

        makePair(LEFT_INDEX_POINT, "LeftIndexPoint"),
        makePair(RIGHT_INDEX_POINT, "RightIndexPoint"),

        makePair(LEFT_PRIMARY_INDEX, "LeftPrimaryIndex"),
        makePair(LEFT_SECONDARY_INDEX, "LeftSecondaryIndex"),
        makePair(RIGHT_PRIMARY_INDEX, "RightPrimaryIndex"),
        makePair(RIGHT_SECONDARY_INDEX, "RightSecondaryIndex"),

        makePair(LEFT_PRIMARY_INDEX_TOUCH, "LeftPrimaryIndexTouch"),
        makePair(LEFT_SECONDARY_INDEX_TOUCH, "LeftSecondaryIndexTouch"),
        makePair(RIGHT_PRIMARY_INDEX_TOUCH, "RightPrimaryIndexTouch"),
        makePair(RIGHT_SECONDARY_INDEX_TOUCH, "RightSecondaryIndexTouch"),

        makePair(LEFT_GRIP, "LeftGrip"),
        makePair(LEFT_GRIP_TOUCH, "LeftGripTouch"),
        makePair(RIGHT_GRIP, "RightGrip"),
        makePair(RIGHT_GRIP_TOUCH, "RightGripTouch"),

        // Poses
        makePair(LEFT_HAND, "LeftHand"),
        makePair(LEFT_HAND_THUMB1, "LeftHandThumb1"),
        makePair(LEFT_HAND_THUMB2, "LeftHandThumb2"),
        makePair(LEFT_HAND_THUMB3, "LeftHandThumb3"),
        makePair(LEFT_HAND_THUMB4, "LeftHandThumb4"),
        makePair(LEFT_HAND_INDEX1, "LeftHandIndex1"),
        makePair(LEFT_HAND_INDEX2, "LeftHandIndex2"),
        makePair(LEFT_HAND_INDEX3, "LeftHandIndex3"),
        makePair(LEFT_HAND_INDEX4, "LeftHandIndex4"),
        makePair(LEFT_HAND_MIDDLE1, "LeftHandMiddle1"),
        makePair(LEFT_HAND_MIDDLE2, "LeftHandMiddle2"),
        makePair(LEFT_HAND_MIDDLE3, "LeftHandMiddle3"),
        makePair(LEFT_HAND_MIDDLE4, "LeftHandMiddle4"),
        makePair(LEFT_HAND_RING1, "LeftHandRing1"),
        makePair(LEFT_HAND_RING2, "LeftHandRing2"),
        makePair(LEFT_HAND_RING3, "LeftHandRing3"),
        makePair(LEFT_HAND_RING4, "LeftHandRing4"),
        makePair(LEFT_HAND_PINKY1, "LeftHandPinky1"),
        makePair(LEFT_HAND_PINKY2, "LeftHandPinky2"),
        makePair(LEFT_HAND_PINKY3, "LeftHandPinky3"),
        makePair(LEFT_HAND_PINKY4, "LeftHandPinky4"),
        makePair(RIGHT_HAND, "RightHand"),
        makePair(RIGHT_HAND_THUMB1, "RightHandThumb1"),
        makePair(RIGHT_HAND_THUMB2, "RightHandThumb2"),
        makePair(RIGHT_HAND_THUMB3, "RightHandThumb3"),
        makePair(RIGHT_HAND_THUMB4, "RightHandThumb4"),
        makePair(RIGHT_HAND_INDEX1, "RightHandIndex1"),
        makePair(RIGHT_HAND_INDEX2, "RightHandIndex2"),
        makePair(RIGHT_HAND_INDEX3, "RightHandIndex3"),
        makePair(RIGHT_HAND_INDEX4, "RightHandIndex4"),
        makePair(RIGHT_HAND_MIDDLE1, "RightHandMiddle1"),
        makePair(RIGHT_HAND_MIDDLE2, "RightHandMiddle2"),
        makePair(RIGHT_HAND_MIDDLE3, "RightHandMiddle3"),
        makePair(RIGHT_HAND_MIDDLE4, "RightHandMiddle4"),
        makePair(RIGHT_HAND_RING1, "RightHandRing1"),
        makePair(RIGHT_HAND_RING2, "RightHandRing2"),
        makePair(RIGHT_HAND_RING3, "RightHandRing3"),
        makePair(RIGHT_HAND_RING4, "RightHandRing4"),
        makePair(RIGHT_HAND_PINKY1, "RightHandPinky1"),
        makePair(RIGHT_HAND_PINKY2, "RightHandPinky2"),
        makePair(RIGHT_HAND_PINKY3, "RightHandPinky3"),
        makePair(RIGHT_HAND_PINKY4, "RightHandPinky4"),
        makePair(LEFT_FOOT, "LeftFoot"),
        makePair(RIGHT_FOOT, "RightFoot"),
        makePair(RIGHT_ARM, "RightArm"),
        makePair(LEFT_ARM, "LeftArm"),
        makePair(HIPS, "Hips"),
        makePair(SPINE2, "Spine2"),
        makePair(HEAD, "Head"),

        // Aliases, PlayStation style names
        makePair(LB, "L1"),
        makePair(RB, "R1"),
        makePair(LT, "L2"),
        makePair(RT, "R2"),
        makePair(LS, "L3"),
        makePair(RS, "R3"),
        makePair(BACK, "Select"),
        makePair(A, "Cross"),
        makePair(B, "Circle"),
        makePair(X, "Square"),
        makePair(Y, "Triangle"),
        makePair(DU, "Up"),
        makePair(DD, "Down"),
        makePair(DL, "Left"),
        makePair(DR, "Right"),

        makePair(TRACKED_OBJECT_00, "TrackedObject00"),
        makePair(TRACKED_OBJECT_01, "TrackedObject01"),
        makePair(TRACKED_OBJECT_02, "TrackedObject02"),
        makePair(TRACKED_OBJECT_03, "TrackedObject03"),
        makePair(TRACKED_OBJECT_04, "TrackedObject04"),
        makePair(TRACKED_OBJECT_05, "TrackedObject05"),
        makePair(TRACKED_OBJECT_06, "TrackedObject06"),
        makePair(TRACKED_OBJECT_07, "TrackedObject07"),
        makePair(TRACKED_OBJECT_08, "TrackedObject08"),
        makePair(TRACKED_OBJECT_09, "TrackedObject09"),
        makePair(TRACKED_OBJECT_10, "TrackedObject10"),
        makePair(TRACKED_OBJECT_11, "TrackedObject11"),
        makePair(TRACKED_OBJECT_12, "TrackedObject12"),
        makePair(TRACKED_OBJECT_13, "TrackedObject13"),
        makePair(TRACKED_OBJECT_14, "TrackedObject14"),
        makePair(TRACKED_OBJECT_15, "TrackedObject15")
    };
    return availableInputs;
}

EndpointPointer StandardController::createEndpoint(const Input& input) const {
    return std::make_shared<StandardEndpoint>(input);
}

QStringList StandardController::getDefaultMappingConfigs() const {
    static const QString DEFAULT_MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/standard.json";
    static const QString DEFAULT_NAV_MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/standard_navigation.json";
    return QStringList() << DEFAULT_NAV_MAPPING_JSON << DEFAULT_MAPPING_JSON;
}

}
