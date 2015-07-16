//
//  laserPointer.js
//  examples
//
//  Created by Clément Brisset on 7/18/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  If using Hydra controllers, pulling the triggers makes laser pointers emanate from the respective hands.
//  If using a Leap Motion or similar to control your avatar's hands and fingers, pointing with your index fingers makes
//  laser pointers emanate from the respective index fingers.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var laserPointer = (function () {

    var NUM_FINGERs = 4,  // Excluding thumb
        fingers = [
            [ "LeftHandIndex", "LeftHandMiddle", "LeftHandRing", "LeftHandPinky" ],
            [ "RightHandIndex", "RightHandMiddle", "RightHandRing", "RightHandPinky" ]
        ];

    function isHandPointing(hand) {
        var MINIMUM_TRIGGER_PULL = 0.9;
        return Controller.getTriggerValue(hand) > MINIMUM_TRIGGER_PULL;
    }

    function isFingerPointing(hand) {
        // Index finger is pointing if final two bones of middle, ring, and pinky fingers are > 90 degrees w.r.t. index finger

        var pointing,
            indexDirection,
            otherDirection,
            f;

        pointing = true;

        indexDirection = Vec3.subtract(
            MyAvatar.getJointPosition(fingers[hand][0] + "4"),
            MyAvatar.getJointPosition(fingers[hand][0] + "2")
        );

        for (f = 1; f < NUM_FINGERs; f += 1) {
            otherDirection = Vec3.subtract(
                MyAvatar.getJointPosition(fingers[hand][f] + "4"),
                MyAvatar.getJointPosition(fingers[hand][f] + "2")
            );
            pointing = pointing && Vec3.dot(indexDirection, otherDirection) < 0;
        }

        return pointing;
    }

    function update() {
        var LEFT_HAND = 0,
            RIGHT_HAND = 1,
            LEFT_HAND_POINTING_FLAG = 1,
            RIGHT_HAND_POINTING_FLAG = 2,
            FINGER_POINTING_FLAG = 4,
            handState;

        handState = 0;

        if (isHandPointing(LEFT_HAND)) {
            handState += LEFT_HAND_POINTING_FLAG;
        }
        if (isHandPointing(RIGHT_HAND)) {
            handState += RIGHT_HAND_POINTING_FLAG;
        }

        if (handState === 0) {
            if (isFingerPointing(LEFT_HAND)) {
                handState += LEFT_HAND_POINTING_FLAG;
            }
            if (isFingerPointing(RIGHT_HAND)) {
                handState += RIGHT_HAND_POINTING_FLAG;
            }
            if (handState !== 0) {
                handState += FINGER_POINTING_FLAG;
            }
        }

        MyAvatar.setHandState(handState);
    }

    return {
        update: update
    };

}());

Script.update.connect(laserPointer.update);
