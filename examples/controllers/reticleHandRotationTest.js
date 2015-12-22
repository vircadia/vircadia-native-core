//
//  reticleHandRotationTest.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var DEBUGGING = false;

Math.clamp=function(a,b,c) {
    return Math.max(b,Math.min(c,a));
}

function length(posA, posB) {
    var dx = posA.x - posB.x;
    var dy = posA.y - posB.y;
    var length = Math.sqrt((dx*dx) + (dy*dy))
    return length;
}

var EXPECTED_CHANGE = 50;
var lastPos = Controller.getReticlePosition();
function moveReticleAbsolute(x, y) {
    var globalPos = Controller.getReticlePosition();
    var dX = x - globalPos.x;
    var dY = y - globalPos.y;

    // some debugging to see if position is jumping around on us...
    var distanceSinceLastMove = length(lastPos, globalPos);
    if (distanceSinceLastMove > EXPECTED_CHANGE) {
        debugPrint("------------------ distanceSinceLastMove:" + distanceSinceLastMove + "----------------------------");
    }

    if (Math.abs(dX) > EXPECTED_CHANGE) {
        debugPrint("surpressing unexpectedly large change dX:" + dX + "----------------------------");
    }
    if (Math.abs(dY) > EXPECTED_CHANGE) {
        debugPrint("surpressing unexpectedly large change dY:" + dY + "----------------------------");
    }

    globalPos.x = x;
    globalPos.y = y;
    Controller.setReticlePosition(globalPos);
    lastPos = globalPos;
}


var MAPPING_NAME = "com.highfidelity.testing.reticleWithHandRotation";
var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from(Controller.Standard.LT).peek().constrainToInteger().to(Controller.Actions.ReticleClick);
mapping.from(Controller.Standard.RT).peek().constrainToInteger().to(Controller.Actions.ReticleClick);
mapping.enable();


var lastRotatedLeft = Vec3.UNIT_NEG_Y;
var lastRotatedRight = Vec3.UNIT_NEG_Y;

function debugPrint(message) {
    if (DEBUGGING) {
        print(message);
    }
}

var MAX_WAKE_UP_DISTANCE = 0.005;
var MIN_WAKE_UP_DISTANCE = 0.001;
var INITIAL_WAKE_UP_DISTANCE = MIN_WAKE_UP_DISTANCE;
var INCREMENTAL_WAKE_UP_DISTANCE = 0.001;

var MAX_SLEEP_DISTANCE = 0.0004;
var MIN_SLEEP_DISTANCE = 0.00001; //0.00002;
var INITIAL_SLEEP_DISTANCE = MIN_SLEEP_DISTANCE;
var INCREMENTAL_SLEEP_DISTANCE = 0.000002; // 0.00002;

var leftAsleep = true;
var rightAsleep = true;

var leftWakeUpDistance = INITIAL_WAKE_UP_DISTANCE;
var rightWakeUpDistance = INITIAL_WAKE_UP_DISTANCE;

var leftSleepDistance = INITIAL_SLEEP_DISTANCE;
var rightSleepDistance = INITIAL_SLEEP_DISTANCE;

Script.update.connect(function(deltaTime) {

    var poseRight = Controller.getPoseValue(Controller.Standard.RightHand);
    var poseLeft = Controller.getPoseValue(Controller.Standard.LeftHand);

    // NOTE: hack for now
    var screenSizeX = 1920;
    var screenSizeY = 1080;

    var rotatedRight = Vec3.multiplyQbyV(poseRight.rotation, Vec3.UNIT_NEG_Y);
    var rotatedLeft = Vec3.multiplyQbyV(poseLeft.rotation, Vec3.UNIT_NEG_Y);

    var suppressRight = false;
    var suppressLeft = false;

    // What I really want to do is to slowly increase the epsilon you have to move it
    // to wake up, the longer you go without moving it
    var leftDistance = Vec3.distance(rotatedLeft, lastRotatedLeft);
    var rightDistance = Vec3.distance(rotatedRight, lastRotatedRight);

    // check to see if hand should wakeup or sleep
    if (leftAsleep) {
        if (leftDistance > leftWakeUpDistance) {
            leftAsleep = false;
            leftSleepDistance = INITIAL_SLEEP_DISTANCE;
            leftWakeUpDistance = INITIAL_WAKE_UP_DISTANCE;
        } else {
            // grow the wake up distance to make it harder to wake up
            leftWakeUpDistance = Math.min(leftWakeUpDistance + INCREMENTAL_WAKE_UP_DISTANCE, MAX_WAKE_UP_DISTANCE);
        }
    } else {
        // we are awake, determine if we should fall asleep, if we haven't moved
        // at least as much as our sleep distance then we sleep
        if (leftDistance < leftSleepDistance) {
            leftAsleep = true;
            leftSleepDistance = INITIAL_SLEEP_DISTANCE;
            leftWakeUpDistance = INITIAL_WAKE_UP_DISTANCE;
        } else {
            // if we moved more than the sleep amount, but we moved less than the max sleep
            // amount, then increase our liklihood of sleep.
            if (leftDistance < MAX_SLEEP_DISTANCE) {
                print("growing sleep....");
                leftSleepDistance = Math.max(leftSleepDistance + INCREMENTAL_SLEEP_DISTANCE, MAX_SLEEP_DISTANCE);
            } else {
                // otherwise reset it to initial 
                leftSleepDistance = INITIAL_SLEEP_DISTANCE;
            }
        }
    }
    if (leftAsleep) {
        suppressLeft = true;
        debugPrint("suppressing left not moving enough");
    }

    // check to see if hand should wakeup or sleep
    if (rightAsleep) {
        if (rightDistance > rightWakeUpDistance) {
            rightAsleep = false;
            rightSleepDistance = INITIAL_SLEEP_DISTANCE;
            rightWakeUpDistance = INITIAL_WAKE_UP_DISTANCE;
        } else {
            // grow the wake up distance to make it harder to wake up
            rightWakeUpDistance = Math.min(rightWakeUpDistance + INCREMENTAL_WAKE_UP_DISTANCE, MAX_WAKE_UP_DISTANCE);
        }
    } else {
        // we are awake, determine if we should fall asleep, if we haven't moved
        // at least as much as our sleep distance then we sleep
        if (rightDistance < rightSleepDistance) {
            rightAsleep = true;
            rightSleepDistance = INITIAL_SLEEP_DISTANCE;
            rightWakeUpDistance = INITIAL_WAKE_UP_DISTANCE;
        } else {
            // if we moved more than the sleep amount, but we moved less than the max sleep
            // amount, then increase our liklihood of sleep.
            if (rightDistance < MAX_SLEEP_DISTANCE) {
                print("growing sleep....");
                rightSleepDistance = Math.max(rightSleepDistance + INCREMENTAL_SLEEP_DISTANCE, MAX_SLEEP_DISTANCE);
            } else {
                // otherwise reset it to initial 
                rightSleepDistance = INITIAL_SLEEP_DISTANCE;
            }
        }
    }
    if (rightAsleep) {
        suppressRight = true;
        debugPrint("suppressing right not moving enough");
    }

    // check to see if hand is on base station
    if (Vec3.equal(rotatedLeft, Vec3.UNIT_NEG_Y)) {
        suppressLeft = true;
        debugPrint("suppressing left on base station");
    }
    if (Vec3.equal(rotatedRight, Vec3.UNIT_NEG_Y)) {
        suppressRight = true;
        debugPrint("suppressing right on base station");
    }

    // Keep track of last rotations, to detect resting (but not on base station hands) in the future
    lastRotatedLeft = rotatedLeft;
    lastRotatedRight = rotatedRight;

    if (suppressLeft && suppressRight) {
        debugPrint("both hands suppressed bail out early");
        return;
    }

    if (suppressLeft) {
        debugPrint("right only");
        rotatedLeft = rotatedRight;
    }
    if (suppressRight) {
        debugPrint("left only");
        rotatedRight = rotatedLeft;
    }

    // Average the two hand positions, if either hand is on base station, the
    // other hand becomes the only used hand and the average is the hand in use
    var rotated = Vec3.multiply(Vec3.sum(rotatedRight,rotatedLeft), 0.5);

    if (DEBUGGING) {
        Vec3.print("rotatedRight:", rotatedRight);
        Vec3.print("rotatedLeft:", rotatedLeft);
        Vec3.print("rotated:", rotated);
    }

    var absolutePitch = rotated.y; // from 1 down to -1 up ... but note: if you rotate down "too far" it starts to go up again...
    var absoluteYaw = -rotated.x; // from -1 left to 1 right

    if (DEBUGGING) {
        print("absolutePitch:" + absolutePitch);
        print("absoluteYaw:" + absoluteYaw);
        Vec3.print("rotated:", rotated);
    }

    var ROTATION_BOUND = 0.6;
    var clampYaw = Math.clamp(absoluteYaw, -ROTATION_BOUND, ROTATION_BOUND);
    var clampPitch = Math.clamp(absolutePitch, -ROTATION_BOUND, ROTATION_BOUND);
    if (DEBUGGING) {
        print("clampYaw:" + clampYaw);
        print("clampPitch:" + clampPitch);
    }

    // using only from -ROTATION_BOUND to ROTATION_BOUND
    var xRatio = (clampYaw + ROTATION_BOUND) / (2 * ROTATION_BOUND);
    var yRatio = (clampPitch + ROTATION_BOUND) / (2 * ROTATION_BOUND);

    if (DEBUGGING) {
        print("xRatio:" + xRatio);
        print("yRatio:" + yRatio);
    }

    var x = screenSizeX * xRatio;
    var y = screenSizeY * yRatio;

    if (DEBUGGING) {
        print("position x:" + x + " y:" + y);
    }
    if (!(xRatio == 0.5 && yRatio == 0)) {
        moveReticleAbsolute(x, y);
    }
});

Script.scriptEnding.connect(function(){
    mapping.disable();
});


