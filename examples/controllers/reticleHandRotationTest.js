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

function moveReticleAbsolute(x, y) {
    var globalPos = Controller.getReticlePosition();
    globalPos.x = x;
    globalPos.y = y;
    Controller.setReticlePosition(globalPos);
}

var MAPPING_NAME = "com.highfidelity.testing.reticleWithHandRotation";
var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from(Controller.Standard.LT).peek().constrainToInteger().to(Controller.Actions.ReticleClick);
mapping.from(Controller.Standard.RT).peek().constrainToInteger().to(Controller.Actions.ReticleClick);
mapping.enable();



function debugPrint(message) {
    if (DEBUGGING) {
        print(message);
    }
}

var leftRightBias = 0.0;
var filteredRotatedLeft = Vec3.UNIT_NEG_Y;
var filteredRotatedRight = Vec3.UNIT_NEG_Y;

Script.update.connect(function(deltaTime) {

    var poseRight = Controller.getPoseValue(Controller.Standard.RightHand);
    var poseLeft = Controller.getPoseValue(Controller.Standard.LeftHand);

    // NOTE: hack for now
    var screenSizeX = 1920;
    var screenSizeY = 1080;

    var rotatedRight = Vec3.multiplyQbyV(poseRight.rotation, Vec3.UNIT_NEG_Y);
    var rotatedLeft = Vec3.multiplyQbyV(poseLeft.rotation, Vec3.UNIT_NEG_Y);

    lastRotatedRight = rotatedRight;


    // Decide which hand should be controlling the pointer
    // by comparing which one is moving more, and by 
    // tending to stay with the one moving more.  
    var BIAS_ADJUST_RATE = 0.5;
    var BIAS_ADJUST_DEADZONE = 0.05;
    leftRightBias += (Vec3.length(poseRight.angularVelocity) - Vec3.length(poseLeft.angularVelocity)) * BIAS_ADJUST_RATE;
    if (leftRightBias < BIAS_ADJUST_DEADZONE) {
        leftRightBias = 0.0;
    } else if (leftRightBias > (1.0 - BIAS_ADJUST_DEADZONE)) {
        leftRightBias = 1.0;
    }

    // Velocity filter the hand rotation used to position reticle so that it is easier to target small things with the hand controllers
    var VELOCITY_FILTER_GAIN = 1.0;
    filteredRotatedLeft = Vec3.mix(filteredRotatedLeft, rotatedLeft, Math.clamp(Vec3.length(poseLeft.angularVelocity) * VELOCITY_FILTER_GAIN, 0.0, 1.0));
    filteredRotatedRight = Vec3.mix(filteredRotatedRight, rotatedRight, Math.clamp(Vec3.length(poseRight.angularVelocity) * VELOCITY_FILTER_GAIN, 0.0, 1.0));
    var rotated = Vec3.mix(filteredRotatedLeft, filteredRotatedRight, leftRightBias);

    var absolutePitch = rotated.y; // from 1 down to -1 up ... but note: if you rotate down "too far" it starts to go up again...
    var absoluteYaw = -rotated.x; // from -1 left to 1 right

    var ROTATION_BOUND = 0.6;
    var clampYaw = Math.clamp(absoluteYaw, -ROTATION_BOUND, ROTATION_BOUND);
    var clampPitch = Math.clamp(absolutePitch, -ROTATION_BOUND, ROTATION_BOUND);

    // using only from -ROTATION_BOUND to ROTATION_BOUND
    var xRatio = (clampYaw + ROTATION_BOUND) / (2 * ROTATION_BOUND);
    var yRatio = (clampPitch + ROTATION_BOUND) / (2 * ROTATION_BOUND);

    var x = screenSizeX * xRatio;
    var y = screenSizeY * yRatio;

    // don't move the reticle with the hand controllers unless the controllers are actually being moved
    var MINIMUM_CONTROLLER_ANGULAR_VELOCITY = 0.0001;
    var angularVelocityMagnitude = Vec3.length(poseLeft.angularVelocity) * (1.0 - leftRightBias) + Vec3.length(poseRight.angularVelocity) * leftRightBias;

    if (!(xRatio == 0.5 && yRatio == 0) && (angularVelocityMagnitude > MINIMUM_CONTROLLER_ANGULAR_VELOCITY)) {
        moveReticleAbsolute(x, y);
    }
});

Script.scriptEnding.connect(function(){
    mapping.disable();
});


