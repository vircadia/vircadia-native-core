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

function length(posA, posB) {
    var dx = posA.x - posB.x;
    var dy = posA.y - posB.y;
    var length = Math.sqrt((dx*dx) + (dy*dy))
    return length;
}

var PITCH_DEADZONE = 1.0;
var PITCH_MAX = 20.0;
var YAW_DEADZONE = 1.0;
var YAW_MAX = 20.0;
var PITCH_SCALING = 10.0; 
var YAW_SCALING = 10.0; 

var EXPECTED_CHANGE = 50;
var lastPos = Reticle.getPosition();
function moveReticle(dY, dX) {
    var globalPos = Reticle.getPosition();

    // some debugging to see if position is jumping around on us...
    var distanceSinceLastMove = length(lastPos, globalPos);
    if (distanceSinceLastMove > EXPECTED_CHANGE) {
        print("distanceSinceLastMove:" + distanceSinceLastMove + "----------------------------");
    }

    if (Math.abs(dX) > EXPECTED_CHANGE) {
        print("UNEXPECTED dX:" + dX + "----------------------------");
        dX = 0;
    }
    if (Math.abs(dY) > EXPECTED_CHANGE) {
        print("UNEXPECTED dY:" + dY + "----------------------------");
        dY = 0;
    }

    globalPos.x += dX;
    globalPos.y += dY;
    Reticle.setPosition(globalPos);
    lastPos = globalPos;
}


var MAPPING_NAME = "com.highfidelity.testing.reticleWithHand";
var mapping = Controller.newMapping(MAPPING_NAME);

var lastHandPitch = 0;
var lastHandYaw = 0;

mapping.from(Controller.Standard.LeftHand).peek().to(function(pose) {
    var handEulers = Quat.safeEulerAngles(pose.rotation);
    //Vec3.print("handEulers:", handEulers);

    var handPitch = handEulers.y;
    var handYaw = handEulers.x;
    var changePitch = (handPitch - lastHandPitch) * PITCH_SCALING;
    var changeYaw = (handYaw - lastHandYaw) * YAW_SCALING;
    if (Math.abs(changePitch) > PITCH_MAX) {
        print("Pitch: " + changePitch);
        changePitch = 0;
    }
    if (Math.abs(changeYaw) > YAW_MAX) {
        print("Yaw: " + changeYaw);
        changeYaw = 0;
    }
    changePitch = Math.abs(changePitch) < PITCH_DEADZONE ? 0 : changePitch;
    changeYaw = Math.abs(changeYaw) < YAW_DEADZONE ? 0 : changeYaw;
    moveReticle(changePitch, changeYaw);  
    lastHandPitch = handPitch;
    lastHandYaw = handYaw;
    
});
mapping.enable();


Script.scriptEnding.connect(function(){
    mapping.disable();
});
