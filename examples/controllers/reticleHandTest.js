//
//  reticleTest.js
//  examples/controllers
//
//  Created by Brad Hefta-Gaub on 2015/12/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


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
var lastPos = Controller.getReticlePosition();
function moveReticle(dX, dY) {
    var globalPos = Controller.getReticlePosition();

    // some debugging to see if position is jumping around on us...
    var distanceSinceLastMove = length(lastPos, globalPos);
    if (distanceSinceLastMove > EXPECTED_CHANGE) {
        print("distanceSinceLastMove:" + distanceSinceLastMove + "----------------------------");
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
    Controller.setReticlePosition(globalPos);
    lastPos = globalPos;
}

var lastTime = msecTimestampNow();

var MAPPING_NAME = "com.highfidelity.testing.reticleWithHand";
var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from(Controller.Standard.RightHand).peek().to(function(pose) {


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


    var MOVE_SCALE = 1;
    var MSECS_PER_SECOND = 1000;
    var now = msecTimestampNow();
    var secondsSinceLast = (now - lastTime) / MSECS_PER_SECOND;
    lastTime = now;

    //print("secondsSinceLast:" + secondsSinceLast);

    //print("x part:" + xPart);
    //print("y part:" + yPart);

    var dX = (xPart * MOVE_SCALE) / secondsSinceLast;
    var dY = (yPart * MOVE_SCALE) / secondsSinceLast;

    //print("dX:" + dX);
    //print("dY:" + dY);

    moveReticle(dX, dY);
});
mapping.enable();


Script.scriptEnding.connect(function(){
    mapping.disable();
});
