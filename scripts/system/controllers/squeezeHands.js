"use strict";

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
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

var lastLeftTrigger = 0;
var lastRightTrigger = 0;
var leftHandOverlayAlpha = 0;
var rightHandOverlayAlpha = 0;

var CONTROLLER_DEAD_SPOT = 0.25;
var TRIGGER_SMOOTH_TIMESCALE = 0.1;
var OVERLAY_RAMP_RATE = 8.0;

var animStateHandlerID;

var leftIndexPointingOverride = 0;
var rightIndexPointingOverride = 0;
var leftThumbRaisedOverride = 0;
var rightThumbRaisedOverride = 0;

var HIFI_POINT_INDEX_MESSAGE_CHANNEL = "Hifi-Point-Index";

var isLeftIndexPointing = false;
var isRightIndexPointing = false;
var isLeftThumbRaised = false;
var isRightThumbRaised = false;

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
        [
            "leftHandOverlayAlpha", "leftHandGraspAlpha",
            "rightHandOverlayAlpha", "rightHandGraspAlpha",
            "isLeftHandGrasp", "isLeftIndexPoint", "isLeftThumbRaise", "isLeftIndexPointAndThumbRaise",
            "isRightHandGrasp", "isRightIndexPoint", "isRightThumbRaise", "isRightIndexPointAndThumbRaise"
        ]
    );
    Messages.subscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
    Messages.messageReceived.connect(handleMessages);
}

function animStateHandler(props) {
    return {
        leftHandOverlayAlpha: leftHandOverlayAlpha,
        leftHandGraspAlpha: lastLeftTrigger,
        rightHandOverlayAlpha: rightHandOverlayAlpha,
        rightHandGraspAlpha: lastRightTrigger,

        isLeftHandGrasp: !isLeftIndexPointing && !isLeftThumbRaised,
        isLeftIndexPoint: isLeftIndexPointing && !isLeftThumbRaised,
        isLeftThumbRaise: !isLeftIndexPointing && isLeftThumbRaised,
        isLeftIndexPointAndThumbRaise: isLeftIndexPointing && isLeftThumbRaised,

        isRightHandGrasp: !isRightIndexPointing && !isRightThumbRaised,
        isRightIndexPoint: isRightIndexPointing && !isRightThumbRaised,
        isRightThumbRaise: !isRightIndexPointing && isRightThumbRaised,
        isRightIndexPointAndThumbRaise: isRightIndexPointing && isRightThumbRaised
    };
}

function update(dt) {
    var leftTrigger = clamp(Controller.getValue(Controller.Standard.LT) + Controller.getValue(Controller.Standard.LeftGrip), 0, 1);
    var rightTrigger = clamp(Controller.getValue(Controller.Standard.RT) + Controller.getValue(Controller.Standard.RightGrip), 0, 1);

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

    // Pointing index fingers and raising thumbs
    isLeftIndexPointing = (leftIndexPointingOverride > 0) || (leftHandPose.valid && Controller.getValue(Controller.Standard.LeftIndexPoint) === 1);
    isRightIndexPointing = (rightIndexPointingOverride > 0) || (rightHandPose.valid && Controller.getValue(Controller.Standard.RightIndexPoint) === 1);
    isLeftThumbRaised = (leftThumbRaisedOverride > 0) || (leftHandPose.valid && Controller.getValue(Controller.Standard.LeftThumbUp) === 1);
    isRightThumbRaised = (rightThumbRaisedOverride > 0) || (rightHandPose.valid && Controller.getValue(Controller.Standard.RightThumbUp) === 1);
}

function handleMessages(channel, message, sender) {
    if (sender === MyAvatar.sessionUUID && channel === HIFI_POINT_INDEX_MESSAGE_CHANNEL) {
        var data = JSON.parse(message);

        if (data.pointIndex !== undefined) {
            if (data.pointIndex) {
                leftIndexPointingOverride++;
                rightIndexPointingOverride++;
            } else {
                leftIndexPointingOverride--;
                rightIndexPointingOverride--;
            }
        }
        if (data.pointLeftIndex !== undefined) {
            if (data.pointLeftIndex) {
                leftIndexPointingOverride++;
            } else {
                leftIndexPointingOverride--;
            }
        }
        if (data.pointRightIndex !== undefined) {
            if (data.pointRightIndex) {
                rightIndexPointingOverride++;
            } else {
                rightIndexPointingOverride--;
            }
        }
        if (data.raiseThumbs !== undefined) {
            if (data.raiseThumbs) {
                leftThumbRaisedOverride++;
                rightThumbRaisedOverride++;
            } else {
                leftThumbRaisedOverride--;
                rightThumbRaisedOverride--;
            }
        }
        if (data.raiseLeftThumb !== undefined) {
            if (data.raiseLeftThumb) {
                leftThumbRaisedOverride++;
            } else {
                leftThumbRaisedOverride--;
            }
        }
        if (data.raiseRightThumb !== undefined) {
            if (data.raiseRightThumb) {
                rightThumbRaisedOverride++;
            } else {
                rightThumbRaisedOverride--;
            }
        }
    }
}

function shutdown() {
    Script.update.disconnect(update);
    MyAvatar.removeAnimationStateHandler(animStateHandlerID);
    Messages.unsubscribe(HIFI_POINT_INDEX_MESSAGE_CHANNEL);
    Messages.messageReceived.disconnect(handleMessages);
}

Script.scriptEnding.connect(shutdown);

init();

}()); // END LOCAL_SCOPE
