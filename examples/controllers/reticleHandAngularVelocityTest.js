//
//  reticleHandAngularVelocityTest.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// If you set this to true, you will get the raw instantaneous angular velocity. 
// note: there is a LOT of noise in the hydra rotation, you will probably be very
// frustrated with the level of jitter.
var USE_INSTANTANEOUS_ANGULAR_VELOCITY = false;
var whichHand = Controller.Standard.RightHand;
var whichTrigger = Controller.Standard.RT;



function msecTimestampNow() {
    var d = new Date();
    return d.getTime();
}

function length(posA, posB) {
    var dx = posA.x - posB.x;
    var dy = posA.y - posB.y;
    var length = Math.sqrt((dx*dx) + (dy*dy))
    return length;
}

var EXPECTED_CHANGE = 50;
var lastPos = Reticle.getPosition();
function moveReticle(dX, dY) {
    var globalPos = Reticle.getPosition();

    // some debugging to see if position is jumping around on us...
    var distanceSinceLastMove = length(lastPos, globalPos);
    if (distanceSinceLastMove > EXPECTED_CHANGE) {
        print("------------------ distanceSinceLastMove:" + distanceSinceLastMove + "----------------------------");
    }

    if (Math.abs(dX) > EXPECTED_CHANGE) {
        print("surpressing unexpectedly large change dX:" + dX + "----------------------------");
        dX = 0;
    }
    if (Math.abs(dY) > EXPECTED_CHANGE) {
        print("surpressing unexpectedly large change dY:" + dY + "----------------------------");
        dY = 0;
    }

    globalPos.x += dX;
    globalPos.y += dY;
    Reticle.setPosition(globalPos);
    lastPos = globalPos;
}

var firstTime = true;
var lastTime = msecTimestampNow();
var previousRotation;

var MAPPING_NAME = "com.highfidelity.testing.reticleWithHand";
var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from(whichTrigger).peek().constrainToInteger().to(Controller.Actions.ReticleClick);
mapping.from(whichHand).peek().to(function(pose) {

    var MSECS_PER_SECOND = 1000;
    var now = msecTimestampNow();
    var deltaMsecs =  (now - lastTime);
    var deltaTime = deltaMsecs / MSECS_PER_SECOND;

    if (firstTime) {
        previousRotation = pose.rotation;
        lastTime = msecTimestampNow();
        firstTime = false;
    }

    // pose.angularVelocity - is the angularVelocity in a "physics" sense, that
    // means the direction of the vector is the axis of symetry of rotation
    // and the scale of the vector is the speed in radians/second of rotation
    // around that axis.
    //
    // we want to deconstruct that in the portion of the rotation on the Y axis
    // and make that portion move our reticle in the horizontal/X direction
    // and the portion of the rotation on the X axis and make that portion 
    // move our reticle in the veritcle/Y direction
    var xPart = -pose.angularVelocity.y;
    var yPart = -pose.angularVelocity.x;

    // pose.angularVelocity is "smoothed", we can calculate our own instantaneous
    // angular velocity as such:
    if (USE_INSTANTANEOUS_ANGULAR_VELOCITY) {
        var previousConjugate = Quat.conjugate(previousRotation);
        var deltaRotation = Quat.multiply(pose.rotation, previousConjugate);
        var normalizedDeltaRotation = Quat.normalize(deltaRotation);
        var axis = Quat.axis(normalizedDeltaRotation);
        var speed = Quat.angle(normalizedDeltaRotation) / deltaTime;
        var instantaneousAngularVelocity = Vec3.multiply(speed, axis);

        xPart = -instantaneousAngularVelocity.y;
        yPart = -instantaneousAngularVelocity.x;

        previousRotation = pose.rotation;
    }

    var MOVE_SCALE = 1;
    lastTime = now;

    var dX = (xPart * MOVE_SCALE) / deltaTime;
    var dY = (yPart * MOVE_SCALE) / deltaTime;

    moveReticle(dX, dY);
});
mapping.enable();

Script.scriptEnding.connect(function(){
    mapping.disable();
});


