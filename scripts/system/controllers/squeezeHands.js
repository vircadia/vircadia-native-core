//
//  controllers/squeezeHands.js
//
//  Created by Anthony J. Thibault
//  Copyright 2015 High Fidelity, Inc.
//
//  Default script to drive the animation of the hands based on hand controllers.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var lastLeftTrigger = 0;
var lastRightTrigger = 0;
var leftHandOverlayAlpha = 0;
var rightHandOverlayAlpha = 0;

var CONTROLLER_DEAD_SPOT = 0.25;
var TRIGGER_SMOOTH_TIMESCALE = 0.1;
var OVERLAY_RAMP_RATE = 8.0;

var animStateHandlerID;

function clamp(val, min, max) {
    return Math.min(Math.max(val, min), max);
}

function normalizeControllerValue(val) {
    return clamp((val - CONTROLLER_DEAD_SPOT) / (1 - CONTROLLER_DEAD_SPOT), 0, 1);
}

function lerp(a, b, alpha) {
    return a * (1 - alpha) + b * alpha;
}

function init() {
    Script.update.connect(update);
    animStateHandlerID = MyAvatar.addAnimationStateHandler(
        animStateHandler,
        ["leftHandOverlayAlpha", "rightHandOverlayAlpha", "leftHandGraspAlpha", "rightHandGraspAlpha"]
    );
}

function animStateHandler(props) {
    return { leftHandOverlayAlpha: leftHandOverlayAlpha,
             leftHandGraspAlpha: lastLeftTrigger,
             rightHandOverlayAlpha: rightHandOverlayAlpha,
             rightHandGraspAlpha: lastRightTrigger };
}

function update(dt) {
    var leftTrigger = normalizeControllerValue(Controller.getValue(Controller.Standard.LT));
    var rightTrigger = normalizeControllerValue(Controller.getValue(Controller.Standard.RT));

    //  Average last few trigger values together for a bit of smoothing
    var tau = clamp(dt / TRIGGER_SMOOTH_TIMESCALE, 0, 1);
    lastLeftTrigger = lerp(leftTrigger, lastLeftTrigger, tau);
    lastRightTrigger = lerp(rightTrigger, lastRightTrigger, tau);

    // ramp on/off left hand overlay
    var leftHandPose = Controller.getPoseValue(Controller.Standard.LeftHand);
    if (leftHandPose.valid) {
        leftHandOverlayAlpha = clamp(leftHandOverlayAlpha + OVERLAY_RAMP_RATE * dt, 0, 1);
    } else {
        leftHandOverlayAlpha = clamp(leftHandOverlayAlpha - OVERLAY_RAMP_RATE * dt, 0, 1);
    }

    // ramp on/off right hand overlay
    var rightHandPose = Controller.getPoseValue(Controller.Standard.RightHand);
    if (rightHandPose.valid) {
        rightHandOverlayAlpha = clamp(rightHandOverlayAlpha + OVERLAY_RAMP_RATE * dt, 0, 1);
    } else {
        rightHandOverlayAlpha = clamp(rightHandOverlayAlpha - OVERLAY_RAMP_RATE * dt, 0, 1);
    }
}

function shutdown() {
    Script.update.disconnect(update);
    MyAvatar.removeAnimationStateHandler(animStateHandlerID);
}

Script.scriptEnding.connect(shutdown);

init();
