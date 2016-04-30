//
//  handControllerMouse.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var DEBUGGING = false;
var angularVelocityTrailingAverage = 0.0;  //  Global trailing average used to decide whether to move reticle at all
var lastX = 0;
var lastY = 0;

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
    var globalPos = Reticle.getPosition();
    globalPos.x = x;
    globalPos.y = y;
    Reticle.setPosition(globalPos);
}

var MAPPING_NAME = "com.highfidelity.testing.reticleWithHandRotation";
var mapping = Controller.newMapping(MAPPING_NAME);
if (Controller.Hardware.Hydra !== undefined) {
    mapping.from(Controller.Hardware.Hydra.L3).peek().to(Controller.Actions.ReticleClick);
    mapping.from(Controller.Hardware.Hydra.R4).peek().to(Controller.Actions.ReticleClick);
}
if (Controller.Hardware.Vive !== undefined) {
    mapping.from(Controller.Hardware.Vive.LeftPrimaryThumb).peek().to(Controller.Actions.ReticleClick);
    mapping.from(Controller.Hardware.Vive.RightPrimaryThumb).peek().to(Controller.Actions.ReticleClick);
}

mapping.enable();

function debugPrint(message) {
    if (DEBUGGING) {
        print(message);
    }
}

var leftRightBias = 0.0;
var filteredRotatedLeft = Vec3.UNIT_NEG_Y;
var filteredRotatedRight = Vec3.UNIT_NEG_Y;
var lastAlpha = 0;

Script.update.connect(function(deltaTime) {

    // avatar frame
    var poseRight = Controller.getPoseValue(Controller.Standard.RightHand);
    var poseLeft = Controller.getPoseValue(Controller.Standard.LeftHand);

    // NOTE: hack for now
    var screenSize = Reticle.maximumPosition;
    var screenSizeX = screenSize.x;
    var screenSizeY = screenSize.y;

    // transform hand facing vectors from avatar frame into sensor frame.
    var worldToSensorMatrix = Mat4.inverse(MyAvatar.sensorToWorldMatrix);
    var rotatedRight = Mat4.transformVector(worldToSensorMatrix, Vec3.multiplyQbyV(MyAvatar.orientation, Vec3.multiplyQbyV(poseRight.rotation, Vec3.UNIT_NEG_Y)));
    var rotatedLeft = Mat4.transformVector(worldToSensorMatrix, Vec3.multiplyQbyV(MyAvatar.orientation, Vec3.multiplyQbyV(poseLeft.rotation, Vec3.UNIT_NEG_Y)));

    lastRotatedRight = rotatedRight;

    // Decide which hand should be controlling the pointer
    // by comparing which one is moving more, and by
    // tending to stay with the one moving more.
    if (deltaTime > 0.001) {
        // leftRightBias is a running average of the difference in angular hand speed.
        // a positive leftRightBias indicates the right hand is spinning faster then the left hand.
        // a negative leftRightBias indicates the left hand is spnning faster.
        var BIAS_ADJUST_PERIOD = 1.0;
        var tau = Math.clamp(deltaTime / BIAS_ADJUST_PERIOD, 0, 1);
        newLeftRightBias = Vec3.length(poseRight.angularVelocity) - Vec3.length(poseLeft.angularVelocity);
        leftRightBias = (1 - tau) * leftRightBias + tau * newLeftRightBias;
    }

    // add a bit of hysteresis to prevent control flopping back and forth
    // between hands when they are both mostly stationary.
    var alpha;
    var HYSTERESIS_OFFSET = 0.25;
    if (lastAlpha > 0.5) {
        // prefer right hand over left
        alpha = leftRightBias > -HYSTERESIS_OFFSET ? 1 : 0;
    } else {
        alpha = leftRightBias > HYSTERESIS_OFFSET ? 1 : 0;
    }
    lastAlpha = alpha;

    // Velocity filter the hand rotation used to position reticle so that it is easier to target small things with the hand controllers
    var VELOCITY_FILTER_GAIN = 0.5;
    filteredRotatedLeft = Vec3.mix(filteredRotatedLeft, rotatedLeft, Math.clamp(Vec3.length(poseLeft.angularVelocity) * VELOCITY_FILTER_GAIN, 0.0, 1.0));
    filteredRotatedRight = Vec3.mix(filteredRotatedRight, rotatedRight, Math.clamp(Vec3.length(poseRight.angularVelocity) * VELOCITY_FILTER_GAIN, 0.0, 1.0));
    var rotated = Vec3.mix(filteredRotatedLeft, filteredRotatedRight, alpha);

    var absolutePitch = rotated.y; // from 1 down to -1 up ... but note: if you rotate down "too far" it starts to go up again...
    var absoluteYaw = -rotated.x; // from -1 left to 1 right

    var x = Math.clamp(screenSizeX * (absoluteYaw + 0.5), 0, screenSizeX);
    var y = Math.clamp(screenSizeX * absolutePitch, 0, screenSizeY);

    // don't move the reticle with the hand controllers unless the controllers are actually being moved
    //  take a time average of angular velocity, and don't move mouse at all if it's below threshold

    var AVERAGING_INTERVAL = 0.95;
    var MINIMUM_CONTROLLER_ANGULAR_VELOCITY = 0.03;
    var angularVelocityMagnitude = Vec3.length(poseLeft.angularVelocity) * (1.0 - alpha) + Vec3.length(poseRight.angularVelocity) * alpha;
    angularVelocityTrailingAverage = angularVelocityTrailingAverage * AVERAGING_INTERVAL + angularVelocityMagnitude * (1.0 - AVERAGING_INTERVAL);

    if ((angularVelocityTrailingAverage > MINIMUM_CONTROLLER_ANGULAR_VELOCITY) && ((x != lastX) || (y != lastY))) {
        moveReticleAbsolute(x, y);
        lastX = x;
        lastY = y;
    }
});

Script.scriptEnding.connect(function(){
    mapping.disable();
});
