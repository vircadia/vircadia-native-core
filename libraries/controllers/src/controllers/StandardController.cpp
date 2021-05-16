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

/*@jsdoc
 * <p>The <code>Controller.Standard</code> object has properties representing standard controller outputs. Those for physical 
 * controllers are based on the XBox controller, with aliases for PlayStation. The property values are integer IDs, uniquely 
 * identifying each output. <em>Read-only.</em></p>
 * <p>These outputs can be mapped to actions or functions in a {@link RouteObject} mapping. The data value provided by each 
 * control is either a number or a {@link Pose}. Numbers are typically normalized to <code>0.0</code> or <code>1.0</code> for 
 * button states, the range <code>0.0</code> &ndash; <code>1.0</code> for unidirectional scales, and the range 
 * <code>-1.0</code> &ndash; <code>1.0</code> for bidirectional scales.</p>
 * <p>Each hardware device has a mapping from its outputs to a subset of <code>Controller.Standard</code> items, specified in a 
 * JSON file. For example, 
 * <a href="https://github.com/highfidelity/hifi/blob/master/interface/resources/controllers/vive.json">vive.json</a>
 * and <a href="https://github.com/highfidelity/hifi/blob/master/interface/resources/controllers/leapmotion.json">
 * leapmotion.json</a>.</p>
 *
 * <table>
 *   <thead>
 *       <tr><th>Property</th><th>Type</th><th>Data</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *
 *     <tr><td colspan="4"><strong>Buttons</strong></td></tr>
 *     <tr><td><code>A</code></td><td>number</td><td>number</td><td>"A" button pressed.</td></tr>
 *     <tr><td><code>B</code></td><td>number</td><td>number</td><td>"B" button pressed.</td></tr>
 *     <tr><td><code>X</code></td><td>number</td><td>number</td><td>"X" button pressed.</td></tr>
 *     <tr><td><code>Y</code></td><td>number</td><td>number</td><td>"Y" button pressed.</td></tr>
 *     <tr><td><code>DL</code></td><td>number</td><td>number</td><td>D-pad left pressed.</td></tr>
 *     <tr><td><code>DR</code></td><td>number</td><td>number</td><td>D-pad right pressed.</td></tr>
 *     <tr><td><code>DU</code></td><td>number</td><td>number</td><td>D-pad up pressed.</td></tr>
 *     <tr><td><code>DD</code></td><td>number</td><td>number</td><td>D-pad down pressed.</td></tr>
 *     <tr><td><code>Start</code></td><td>number</td><td>number</td><td>"Start" center button pressed.</td></tr>
 *     <tr><td><code>Back</code></td><td>number</td><td>number</td><td>"Back" center button pressed.</td></tr>
 *     <tr><td><code>LB</code></td><td>number</td><td>number</td><td>Left bumper button pressed.</td></tr>
 *     <tr><td><code>RB</code></td><td>number</td><td>number</td><td>Right bumper button pressed.</td></tr>
 *
 *     <tr><td colspan="4"><strong>Sticks</strong></td></tr>
 *     <tr><td><code>LX</code></td><td>number</td><td>number</td><td>Left stick x-axis scale.</td></tr>
 *     <tr><td><code>LY</code></td><td>number</td><td>number</td><td>Left stick y-axis scale.</td></tr>
 *     <tr><td><code>RX</code></td><td>number</td><td>number</td><td>Right stick x-axis scale.</td></tr>
 *     <tr><td><code>RY</code></td><td>number</td><td>number</td><td>Right stick y-axis scale.</td></tr>
 *     <tr><td><code>LS</code></td><td>number</td><td>number</td><td>Left stick button pressed.</td></tr>
 *     <tr><td><code>RS</code></td><td>number</td><td>number</td><td>Right stick button pressed.</td></tr>
 *     <tr><td><code>LSTouch</code></td><td>number</td><td>number</td><td>Left stick is touched.</td></tr>
 *     <tr><td><code>RSTouch</code></td><td>number</td><td>number</td><td>Right stick is touched.</td></tr>
 *
 *     <tr><td colspan="4"><strong>Triggers</strong></td></tr>
 *     <tr><td><code>LT</code></td><td>number</td><td>number</td><td>Left trigger scale.</td></tr>
 *     <tr><td><code>RT</code></td><td>number</td><td>number</td><td>Right trigger scale.</td></tr>
 *     <tr><td><code>LTClick</code></td><td>number</td><td>number</td><td>Left trigger click.</td></tr>
 *     <tr><td><code>RTClick</code></td><td>number</td><td>number</td><td>Right trigger click.</td></tr>
 *     <tr><td><code>LeftGrip</code></td><td>number</td><td>number</td><td>Left grip scale.</td></tr>
 *     <tr><td><code>RightGrip</code></td><td>number</td><td>number</td><td>Right grip scale.</td></tr>
 *     <tr><td><code>LeftGripTouch</code></td><td>number</td><td>number</td><td>Left grip is touched.</td></tr>
 *     <tr><td><code>RightGripTouch</code></td><td>number</td><td>number</td><td>Right grip is touched.</td></tr>
 *
 *     <tr><td colspan="4"><strong>Aliases, PlayStation Style Names</strong></td></tr>
 *     <tr><td><code>Cross</code></td><td>number</td><td>number</td><td>Alias for <code>A</code>.</td></tr>
 *     <tr><td><code>Circle</code></td><td>number</td><td>number</td><td>Alias for <code>B</code>.</td></tr>
 *     <tr><td><code>Square</code></td><td>number</td><td>number</td><td>Alias for <code>X</code>.</td></tr>
 *     <tr><td><code>Triangle</code></td><td>number</td><td>number</td><td>Alias for <code>Y</code>.</td></tr>
 *     <tr><td><code>Left</code></td><td>number</td><td>number</td><td>Alias for <code>DL</code>.</td></tr>
 *     <tr><td><code>Right</code></td><td>number</td><td>number</td><td>Alias for <code>DR</code>.</td></tr>
 *     <tr><td><code>Up</code></td><td>number</td><td>number</td><td>Alias for <code>DU</code>.</td></tr>
 *     <tr><td><code>Down</code></td><td>number</td><td>number</td><td>Alias for <code>DD</code>.</td></tr>
 *     <tr><td><code>Select</code></td><td>number</td><td>number</td><td>Alias for <code>Back</code>.</td></tr>
 *     <tr><td><code>L1</code></td><td>number</td><td>number</td><td>Alias for <code>LB</code>.</td></tr>
 *     <tr><td><code>R1</code></td><td>number</td><td>number</td><td>Alias for <code>RB</code>.</td></tr>
 *     <tr><td><code>L3</code></td><td>number</td><td>number</td><td>Alias for <code>LS</code>.</td></tr>
 *     <tr><td><code>R3</code></td><td>number</td><td>number</td><td>Alias for <code>RS</code>.</td></tr>
 *     <tr><td><code>L2</code></td><td>number</td><td>number</td><td>Alias for <code>LT</code>.</td></tr>
 *     <tr><td><code>R2</code></td><td>number</td><td>number</td><td>Alias for <code>RT</code>.</td></tr>
 *
 *     <tr><td colspan="4"><strong>Finger Abstractions</strong></td></tr>
 *     <tr><td><code>LeftPrimaryThumb</code></td><td>number</td><td>number</td><td>Left primary thumb button pressed.</td></tr>
 *     <tr><td><code>LeftSecondaryThumb</code></td><td>number</td><td>number</td><td>Left secondary thumb button pressed.
 *       </td></tr>
 *     <tr><td><code>RightPrimaryThumb</code></td><td>number</td><td>number</td><td>Right primary thumb button pressed.
 *       </td></tr>
 *     <tr><td><code>RightSecondaryThumb</code></td><td>number</td><td>number</td><td>Right secondary thumb button pressed.
 *       </td></tr>
 *     <tr><td><code>LeftPrimaryThumbTouch</code></td><td>number</td><td>number</td><td>Left thumb touching primary thumb 
 *       button.</td></tr>
 *     <tr><td><code>LeftSecondaryThumbTouch</code></td><td>number</td><td>number</td><td>Left thumb touching secondary thumb 
 *       button.</td></tr>
 *     <tr><td><code>LeftThumbUp</code></td><td>number</td><td>number</td><td>Left thumb not touching primary or secondary 
 *       thumb buttons.</td></tr>
 *     <tr><td><code>RightPrimaryThumbTouch</code></td><td>number</td><td>number</td><td>Right thumb touching primary thumb 
 *       button.</td></tr>
 *     <tr><td><code>RightSecondaryThumbTouch</code></td><td>number</td><td>number</td><td>Right thumb touching secondary thumb 
 *       button.</td></tr>
 *     <tr><td><code>RightThumbUp</code></td><td>number</td><td>number</td><td>Right thumb not touching primary or secondary 
 *       thumb buttons.</td></tr>
 *     <tr><td><code>LeftPrimaryIndex</code></td><td>number</td><td>number</td><td>Left primary index control pressed.</td></tr>
 *     <tr><td><code>LeftSecondaryIndex</code></td><td>number</td><td>number</td><td>Left secondary index control pressed.
 *       </td></tr>
 *     <tr><td><code>RightPrimaryIndex</code></td><td>number</td><td>number</td><td>Right primary index control pressed. 
 *       </td></tr>
 *     <tr><td><code>RightSecondaryIndex</code></td><td>number</td><td>number</td><td>Right secondary index control pressed.
 *       </td></tr>
 *     <tr><td><code>LeftPrimaryIndexTouch</code></td><td>number</td><td>number</td><td>Left index finger is touching primary 
 *       index finger control.</td></tr>
 *     <tr><td><code>LeftSecondaryIndexTouch</code></td><td>number</td><td>number</td><td>Left index finger is touching 
 *       secondary index finger control.</td></tr>
 *     <tr><td><code>LeftIndexPoint</code></td><td>number</td><td>number</td><td>Left index finger is pointing, not touching 
 *       primary or secondary index finger controls.</td></tr>
 *     <tr><td><code>RightPrimaryIndexTouch</code></td><td>number</td><td>number</td><td>Right index finger is touching primary 
 *       index finger control.</td></tr>
 *     <tr><td><code>RightSecondaryIndexTouch</code></td><td>number</td><td>number</td><td>Right index finger is touching 
 *       secondary index finger control.</td></tr>
 *     <tr><td><code>RightIndexPoint</code></td><td>number</td><td>number</td><td>Right index finger is pointing, not touching 
 *       primary or secondary index finger controls.</td></tr>
 * 
 *     <tr><td colspan="4"><strong>Avatar Skeleton</strong></td></tr>
 *     <tr><td><code>Hips</code></td><td>number</td><td>{@link Pose}</td><td>Hips pose.</td></tr>
 *     <tr><td><code>Spine2</code></td><td>number</td><td>{@link Pose}</td><td>Spine2 pose.</td></tr>
 *     <tr><td><code>Head</code></td><td>number</td><td>{@link Pose}</td><td>Head pose.</td></tr>
 *     <tr><td><code>LeftArm</code></td><td>number</td><td>{@link Pose}</td><td>Left arm pose.</td></tr>
 *     <tr><td><code>RightArm</code></td><td>number</td><td>{@link Pose}</td><td>Right arm pose</td></tr>
 *     <tr><td><code>LeftHand</code></td><td>number</td><td>{@link Pose}</td><td>Left hand pose.</td></tr>
 *     <tr><td><code>LeftHandThumb1</code></td><td>number</td><td>{@link Pose}</td><td>Left thumb 1 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandThumb2</code></td><td>number</td><td>{@link Pose}</td><td>Left thumb 2 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandThumb3</code></td><td>number</td><td>{@link Pose}</td><td>Left thumb 3 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandThumb4</code></td><td>number</td><td>{@link Pose}</td><td>Left thumb 4 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandIndex1</code></td><td>number</td><td>{@link Pose}</td><td>Left index 1 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandIndex2</code></td><td>number</td><td>{@link Pose}</td><td>Left index 2 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandIndex3</code></td><td>number</td><td>{@link Pose}</td><td>Left index 3 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandIndex4</code></td><td>number</td><td>{@link Pose}</td><td>Left index 4 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandMiddle1</code></td><td>number</td><td>{@link Pose}</td><td>Left middle 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftHandMiddle2</code></td><td>number</td><td>{@link Pose}</td><td>Left middle 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftHandMiddle3</code></td><td>number</td><td>{@link Pose}</td><td>Left middle 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftHandMiddle4</code></td><td>number</td><td>{@link Pose}</td><td>Left middle 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftHandRing1</code></td><td>number</td><td>{@link Pose}</td><td>Left ring 1 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandRing2</code></td><td>number</td><td>{@link Pose}</td><td>Left ring 2 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandRing3</code></td><td>number</td><td>{@link Pose}</td><td>Left ring 3 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandRing4</code></td><td>number</td><td>{@link Pose}</td><td>Left ring 4 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandPinky1</code></td><td>number</td><td>{@link Pose}</td><td>Left pinky 1 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandPinky2</code></td><td>number</td><td>{@link Pose}</td><td>Left pinky 2 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandPinky3</code></td><td>number</td><td>{@link Pose}</td><td>Left pinky 3 finger joint pose.</td></tr>
 *     <tr><td><code>LeftHandPinky4</code></td><td>number</td><td>{@link Pose}</td><td>Left pinky 4 finger joint pose.</td></tr>
 *     <tr><td><code>RightHand</code></td><td>number</td><td>{@link Pose}</td><td>Right hand pose.</td></tr>
 *     <tr><td><code>RightHandThumb1</code></td><td>number</td><td>{@link Pose}</td><td>Right thumb 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandThumb2</code></td><td>number</td><td>{@link Pose}</td><td>Right thumb 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandThumb3</code></td><td>number</td><td>{@link Pose}</td><td>Right thumb 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandThumb4</code></td><td>number</td><td>{@link Pose}</td><td>Right thumb 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandIndex1</code></td><td>number</td><td>{@link Pose}</td><td>Right index 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandIndex2</code></td><td>number</td><td>{@link Pose}</td><td>Right index 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandIndex3</code></td><td>number</td><td>{@link Pose}</td><td>Right index 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandIndex4</code></td><td>number</td><td>{@link Pose}</td><td>Right index 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandMiddle1</code></td><td>number</td><td>{@link Pose}</td><td>Right middle 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandMiddle2</code></td><td>number</td><td>{@link Pose}</td><td>Right middle 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandMiddle3</code></td><td>number</td><td>{@link Pose}</td><td>Right middle 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandMiddle4</code></td><td>number</td><td>{@link Pose}</td><td>Right middle 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandRing1</code></td><td>number</td><td>{@link Pose}</td><td>Right ring 1 finger joint pose.</td></tr>
 *     <tr><td><code>RightHandRing2</code></td><td>number</td><td>{@link Pose}</td><td>Right ring 2 finger joint pose.</td></tr>
 *     <tr><td><code>RightHandRing3</code></td><td>number</td><td>{@link Pose}</td><td>Right ring 3 finger joint pose.</td></tr>
 *     <tr><td><code>RightHandRing4</code></td><td>number</td><td>{@link Pose}</td><td>Right ring 4 finger joint pose.</td></tr>
 *     <tr><td><code>RightHandPinky1</code></td><td>number</td><td>{@link Pose}</td><td>Right pinky 1 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandPinky2</code></td><td>number</td><td>{@link Pose}</td><td>Right pinky 2 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandPinky3</code></td><td>number</td><td>{@link Pose}</td><td>Right pinky 3 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>RightHandPinky4</code></td><td>number</td><td>{@link Pose}</td><td>Right pinky 4 finger joint pose.
 *       </td></tr>
 *     <tr><td><code>LeftFoot</code></td><td>number</td><td>{@link Pose}</td><td>Left foot pose.</td></tr>
 *     <tr><td><code>RightFoot</code></td><td>number</td><td>{@link Pose}</td><td>Right foot pose.</td></tr>
 *
 *     <tr><td colspan="4"><strong>Trackers</strong></td></tr>
 *     <tr><td><code>TrackedObject00</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 0 pose.</td></tr>
 *     <tr><td><code>TrackedObject01</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 1 pose.</td></tr>
 *     <tr><td><code>TrackedObject02</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 2 pose.</td></tr>
 *     <tr><td><code>TrackedObject03</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 3 pose.</td></tr>
 *     <tr><td><code>TrackedObject04</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 4 pose.</td></tr>
 *     <tr><td><code>TrackedObject05</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 5 pose.</td></tr>
 *     <tr><td><code>TrackedObject06</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 6 pose.</td></tr>
 *     <tr><td><code>TrackedObject07</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 7 pose.</td></tr>
 *     <tr><td><code>TrackedObject08</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 8 pose.</td></tr>
 *     <tr><td><code>TrackedObject09</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 9 pose.</td></tr>
 *     <tr><td><code>TrackedObject10</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 10 pose.</td></tr>
 *     <tr><td><code>TrackedObject11</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 11 pose.</td></tr>
 *     <tr><td><code>TrackedObject12</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 12 pose.</td></tr>
 *     <tr><td><code>TrackedObject13</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 13 pose.</td></tr>
 *     <tr><td><code>TrackedObject14</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 14 pose.</td></tr>
 *     <tr><td><code>TrackedObject15</code></td><td>number</td><td>{@link Pose}</td><td>Tracker 15 pose.</td></tr>
 *
 *   </tbody>
 * </table>
 * @typedef {object} Controller.Standard
 */
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
        makePair(LEFT_EYE, "LeftEye"),
        makePair(RIGHT_EYE, "RightEye"),

        // blendshapes
        makePair(EYEBLINK_L, "EyeBlink_L"),
        makePair(EYEBLINK_R, "EyeBlink_R"),
        makePair(EYESQUINT_L, "EyeSquint_L"),
        makePair(EYESQUINT_R, "EyeSquint_R"),
        makePair(EYEDOWN_L, "EyeDown_L"),
        makePair(EYEDOWN_R, "EyeDown_R"),
        makePair(EYEIN_L, "EyeIn_L"),
        makePair(EYEIN_R, "EyeIn_R"),
        makePair(EYEOPEN_L, "EyeOpen_L"),
        makePair(EYEOPEN_R, "EyeOpen_R"),
        makePair(EYEOUT_L, "EyeOut_L"),
        makePair(EYEOUT_R, "EyeOut_R"),
        makePair(EYEUP_L, "EyeUp_L"),
        makePair(EYEUP_R, "EyeUp_R"),
        makePair(BROWSD_L, "BrowsD_L"),
        makePair(BROWSD_R, "BrowsD_R"),
        makePair(BROWSU_C, "BrowsU_C"),
        makePair(BROWSU_L, "BrowsU_L"),
        makePair(BROWSU_R, "BrowsU_R"),
        makePair(JAWFWD, "JawFwd"),
        makePair(JAWLEFT, "JawLeft"),
        makePair(JAWOPEN, "JawOpen"),
        makePair(JAWRIGHT, "JawRight"),
        makePair(MOUTHLEFT, "MouthLeft"),
        makePair(MOUTHRIGHT, "MouthRight"),
        makePair(MOUTHFROWN_L, "MouthFrown_L"),
        makePair(MOUTHFROWN_R, "MouthFrown_R"),
        makePair(MOUTHSMILE_L, "MouthSmile_L"),
        makePair(MOUTHSMILE_R, "MouthSmile_R"),
        makePair(MOUTHDIMPLE_L, "MouthDimple_L"),
        makePair(MOUTHDIMPLE_R, "MouthDimple_R"),
        makePair(LIPSSTRETCH_L, "LipsStretch_L"),
        makePair(LIPSSTRETCH_R, "LipsStretch_R"),
        makePair(LIPSUPPERCLOSE, "LipsUpperClose"),
        makePair(LIPSLOWERCLOSE, "LipsLowerClose"),
        makePair(LIPSFUNNEL, "LipsFunnel"),
        makePair(LIPSPUCKER, "LipsPucker"),
        makePair(PUFF, "Puff"),
        makePair(CHEEKSQUINT_L, "CheekSquint_L"),
        makePair(CHEEKSQUINT_R, "CheekSquint_R"),
        makePair(MOUTHCLOSE, "MouthClose"),
        makePair(MOUTHUPPERUP_L, "MouthUpperUp_L"),
        makePair(MOUTHUPPERUP_R, "MouthUpperUp_R"),
        makePair(MOUTHLOWERDOWN_L, "MouthLowerDown_L"),
        makePair(MOUTHLOWERDOWN_R, "MouthLowerDown_R"),
        makePair(MOUTHPRESS_L, "MouthPress_L"),
        makePair(MOUTHPRESS_R, "MouthPress_R"),
        makePair(MOUTHSHRUGLOWER, "MouthShrugLower"),
        makePair(MOUTHSHRUGUPPER, "MouthShrugUpper"),
        makePair(NOSESNEER_L, "NoseSneer_L"),
        makePair(NOSESNEER_R, "NoseSneer_R"),
        makePair(TONGUEOUT, "TongueOut"),
        makePair(USERBLENDSHAPE0, "UserBlendshape0"),
        makePair(USERBLENDSHAPE1, "UserBlendshape1"),
        makePair(USERBLENDSHAPE2, "UserBlendshape2"),
        makePair(USERBLENDSHAPE3, "UserBlendshape3"),
        makePair(USERBLENDSHAPE4, "UserBlendshape4"),
        makePair(USERBLENDSHAPE5, "UserBlendshape5"),
        makePair(USERBLENDSHAPE6, "UserBlendshape6"),
        makePair(USERBLENDSHAPE7, "UserBlendshape7"),
        makePair(USERBLENDSHAPE8, "UserBlendshape8"),
        makePair(USERBLENDSHAPE9, "UserBlendshape9"),

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
