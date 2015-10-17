//
// squeezeHands.js
// examples
//
//  Created by Philip Rosedale on June 4, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
var HIFI_OZAN_BUCKET = "http://hifi-public.s3.amazonaws.com/ozan/";

var rightHandAnimation = HIFI_OZAN_BUCKET + "anim/squeeze_hands/right_hand_anim.fbx";
var leftHandAnimation = HIFI_OZAN_BUCKET + "anim/squeeze_hands/left_hand_anim.fbx";

var lastLeftTrigger = 0;
var lastRightTrigger = 0;

var leftIsClosing = true;
var rightIsClosing = true;

var LAST_FRAME = 15.0;    //  What is the number of the last frame we want to use in the animation?
var SMOOTH_FACTOR = 0.75;
var MAX_FRAMES = 30.0;

var LEFT_HAND_CLICK = Controller.findAction("LEFT_HAND_CLICK");
var RIGHT_HAND_CLICK = Controller.findAction("RIGHT_HAND_CLICK");

var CONTROLLER_DEAD_SPOT = 0.25;

function clamp(val, min, max) {
    if (val < min) {
        return min;
    } else if (val > max) {
        return max;
    } else {
        return val;
    }
}

function normalizeControllerValue(val) {
    return clamp((val - CONTROLLER_DEAD_SPOT) / (1.0 - CONTROLLER_DEAD_SPOT), 0.0, 1.0);
}

Script.update.connect(function(deltaTime) {
    var leftTrigger = normalizeControllerValue(Controller.getActionValue(LEFT_HAND_CLICK));
    var rightTrigger = normalizeControllerValue(Controller.getActionValue(RIGHT_HAND_CLICK));

    //  Average last few trigger values together for a bit of smoothing
    var smoothLeftTrigger = leftTrigger * (1.0 - SMOOTH_FACTOR) + lastLeftTrigger * SMOOTH_FACTOR;
    var smoothRightTrigger = rightTrigger * (1.0 - SMOOTH_FACTOR) + lastRightTrigger * SMOOTH_FACTOR;

    if (leftTrigger == 0.0) {
        leftIsClosing = true;
    } else if (leftTrigger == 1.0) {
        leftIsClosing = false;
    }

    if (rightTrigger == 0.0) {
        rightIsClosing = true;
    } else if (rightTrigger == 1.0) {
        rightIsClosing = false;
    }

    lastLeftTrigger = smoothLeftTrigger;
    lastRightTrigger = smoothRightTrigger;

    // 0..15
    var leftFrame = smoothLeftTrigger * LAST_FRAME;
    var rightFrame = smoothRightTrigger * LAST_FRAME;

    var adjustedLeftFrame = (leftIsClosing) ? leftFrame : (MAX_FRAMES - leftFrame);
    if (leftHandAnimation.length) {
        MyAvatar.startAnimation(leftHandAnimation, 30.0, 1.0, false, true, adjustedLeftFrame, adjustedLeftFrame);
    }

    var adjustedRightFrame = (rightIsClosing) ? rightFrame : (MAX_FRAMES - rightFrame);
    if (rightHandAnimation.length) {
        MyAvatar.startAnimation(rightHandAnimation, 30.0, 1.0, false, true, adjustedRightFrame, adjustedRightFrame);
    }
});

Script.scriptEnding.connect(function() {
    MyAvatar.stopAnimation(leftHandAnimation);
    MyAvatar.stopAnimation(rightHandAnimation);
});
