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

var whichHand = Controller.Standard.RightHand;
var whichTrigger = Controller.Standard.RT;
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
        print("------------------ distanceSinceLastMove:" + distanceSinceLastMove + "----------------------------");
    }

    if (Math.abs(dX) > EXPECTED_CHANGE) {
        print("surpressing unexpectedly large change dX:" + dX + "----------------------------");
    }
    if (Math.abs(dY) > EXPECTED_CHANGE) {
        print("surpressing unexpectedly large change dY:" + dY + "----------------------------");
    }

    globalPos.x = x;
    globalPos.y = y;
    Controller.setReticlePosition(globalPos);
    lastPos = globalPos;
}


var MAPPING_NAME = "com.highfidelity.testing.reticleWithHandRotation";
var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from(whichTrigger).peek().constrainToInteger().to(Controller.Actions.ReticleClick);
mapping.from(whichHand).peek().to(function(pose) {

    // NOTE: hack for now
    var screenSizeX = 1920;
    var screenSizeY = 1080;

    var rotated = Vec3.multiplyQbyV(pose.rotation, Vec3.UNIT_NEG_Y); // 
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
mapping.enable();

Script.scriptEnding.connect(function(){
    mapping.disable();
});


