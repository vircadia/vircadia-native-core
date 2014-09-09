//
//  headMove.js
//  examples
//
//  Created by Philip Rosedale on September 8, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Press the spacebar and move/turn your head to move around. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var debug = false;

var movingWithHead = false; 
var headStartPosition, headStartDeltaPitch, headStartFinalPitch, headStartRoll, headStartYaw;

var HEAD_MOVE_DEAD_ZONE = 0.0;
var HEAD_STRAFE_DEAD_ZONE = 0.0;
var HEAD_ROTATE_DEAD_ZONE = 0.0; 
var HEAD_THRUST_FWD_SCALE = 12000.0;
var HEAD_THRUST_STRAFE_SCALE = 1000.0;
var HEAD_YAW_RATE = 2.0;
var HEAD_PITCH_RATE = 1.0;
var HEAD_ROLL_THRUST_SCALE = 75.0;
var HEAD_PITCH_LIFT_THRUST = 3.0;

function moveWithHead(deltaTime) {
    if (movingWithHead) {
        var deltaYaw = MyAvatar.getHeadFinalYaw() - headStartYaw;
        var deltaPitch = MyAvatar.getHeadDeltaPitch() - headStartDeltaPitch;

        var bodyLocalCurrentHeadVector = Vec3.subtract(MyAvatar.getHeadPosition(), MyAvatar.position);
        bodyLocalCurrentHeadVector = Vec3.multiplyQbyV(Quat.angleAxis(-deltaYaw, {x:0, y: 1, z:0}), bodyLocalCurrentHeadVector);
        var headDelta = Vec3.subtract(bodyLocalCurrentHeadVector, headStartPosition);
        headDelta = Vec3.multiplyQbyV(Quat.inverse(Camera.getOrientation()), headDelta);
        headDelta.y = 0.0;   //  Don't respond to any of the vertical component of head motion

        //  Thrust based on leaning forward and side-to-side
        if (Math.abs(headDelta.z) > HEAD_MOVE_DEAD_ZONE) {
            MyAvatar.addThrust(Vec3.multiply(Quat.getFront(Camera.getOrientation()), -headDelta.z * HEAD_THRUST_FWD_SCALE * deltaTime));
        }
        if (Math.abs(headDelta.x) > HEAD_STRAFE_DEAD_ZONE) {
            MyAvatar.addThrust(Vec3.multiply(Quat.getRight(Camera.getOrientation()), headDelta.x * HEAD_THRUST_STRAFE_SCALE * deltaTime));
        }
        if (Math.abs(deltaYaw) > HEAD_ROTATE_DEAD_ZONE) {
            var orientation = Quat.multiply(Quat.angleAxis(deltaYaw * HEAD_YAW_RATE * deltaTime, {x:0, y: 1, z:0}), MyAvatar.orientation);
            MyAvatar.orientation = orientation;
        }
        //  Thrust Up/Down based on head pitch
        MyAvatar.addThrust(Vec3.multiply({ x:0, y:1, z:0 }, (MyAvatar.getHeadFinalPitch() - headStartFinalPitch) * HEAD_PITCH_LIFT_THRUST * deltaTime));
        //  For head trackers, adjust pitch by head pitch
        MyAvatar.headPitch += deltaPitch * HEAD_PITCH_RATE * deltaTime; 
        //  Thrust strafe based on roll ange 
        MyAvatar.addThrust(Vec3.multiply(Quat.getRight(Camera.getOrientation()), -(MyAvatar.getHeadFinalRoll() - headStartRoll) * HEAD_ROLL_THRUST_SCALE * deltaTime));
    }
}

Controller.keyPressEvent.connect(function(event) {
    if (event.text == "SPACE" && !movingWithHead) {
        movingWithHead = true;
        headStartPosition = Vec3.subtract(MyAvatar.getHeadPosition(), MyAvatar.position);
        headStartDeltaPitch = MyAvatar.getHeadDeltaPitch();
        headStartFinalPitch = MyAvatar.getHeadFinalPitch();
        headStartRoll = MyAvatar.getHeadFinalRoll();
        headStartYaw = MyAvatar.getHeadFinalYaw(); 
    }                        
});
Controller.keyReleaseEvent.connect(function(event) {
    if (event.text == "SPACE") {
        movingWithHead = false;
    }                        
});

Script.update.connect(moveWithHead);

