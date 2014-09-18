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
var HEAD_THRUST_STRAFE_SCALE = 2000.0;
var HEAD_YAW_RATE = 1.0;
var HEAD_PITCH_RATE = 1.0;
var HEAD_ROLL_THRUST_SCALE = 75.0;
var HEAD_PITCH_LIFT_THRUST = 3.0;
var WALL_BOUNCE = 4000.0;

//  If these values are set to something 
var maxVelocity = 1.25;
var noFly = true; 

//var roomLimits = { xMin: 618, xMax: 635.5, zMin: 528, zMax: 552.5 };
var roomLimits = { xMin: -1, xMax: 0, zMin: 0, zMax: 0 };

function isInRoom(position) {
    var BUFFER = 2.0;
    if (roomLimits.xMin < 0) {
        return false; 
    }
    if ((position.x > (roomLimits.xMin - BUFFER)) && 
        (position.x < (roomLimits.xMax + BUFFER)) && 
        (position.z > (roomLimits.zMin - BUFFER)) &&
        (position.z < (roomLimits.zMax + BUFFER))) 
    {
        return true;
    } else {
        return false;
    }
}

function moveWithHead(deltaTime) {
    var thrust = { x: 0, y: 0, z: 0 };
    var position = MyAvatar.position;
    if (movingWithHead) {
        var deltaYaw = MyAvatar.getHeadFinalYaw() - headStartYaw;
        var deltaPitch = MyAvatar.getHeadDeltaPitch() - headStartDeltaPitch;
        var deltaRoll = MyAvatar.getHeadFinalRoll() - headStartRoll;
        var velocity = MyAvatar.getVelocity();
        var bodyLocalCurrentHeadVector = Vec3.subtract(MyAvatar.getHeadPosition(), MyAvatar.position);
        bodyLocalCurrentHeadVector = Vec3.multiplyQbyV(Quat.angleAxis(-deltaYaw, {x:0, y: 1, z:0}), bodyLocalCurrentHeadVector);
        var headDelta = Vec3.subtract(bodyLocalCurrentHeadVector, headStartPosition);
        headDelta = Vec3.multiplyQbyV(Quat.inverse(Camera.getOrientation()), headDelta);
        headDelta.y = 0.0;   //  Don't respond to any of the vertical component of head motion

        var forward = Quat.getFront(Camera.getOrientation());
        var right = Quat.getRight(Camera.getOrientation());
        var up = Quat.getUp(Camera.getOrientation());
        if (noFly) {
            forward.y = 0.0;
            forward = Vec3.normalize(forward);
            right.y = 0.0;
            right = Vec3.normalize(right);
            up = { x: 0, y: 1, z: 0};
        }

        //  Thrust based on leaning forward and side-to-side
        if (Math.abs(headDelta.z) > HEAD_MOVE_DEAD_ZONE) {
            if (Math.abs(Vec3.dot(velocity, forward)) < maxVelocity) {
                thrust = Vec3.sum(thrust, Vec3.multiply(forward, -headDelta.z * HEAD_THRUST_FWD_SCALE * deltaTime));
            } 
        }
        if (Math.abs(headDelta.x) > HEAD_STRAFE_DEAD_ZONE) {
            if (Math.abs(Vec3.dot(velocity, right)) < maxVelocity) {
                thrust = Vec3.sum(thrust, Vec3.multiply(right, headDelta.x * HEAD_THRUST_STRAFE_SCALE * deltaTime));
            }
        }
        if (Math.abs(deltaYaw) > HEAD_ROTATE_DEAD_ZONE) {
            var orientation = Quat.multiply(Quat.angleAxis((deltaYaw + deltaRoll) * HEAD_YAW_RATE * deltaTime, {x:0, y: 1, z:0}), MyAvatar.orientation);
            MyAvatar.orientation = orientation;
        }
        //  Thrust Up/Down based on head pitch
        if (!noFly) {
            if ((Math.abs(Vec3.dot(velocity, up)) < maxVelocity)) {
                thrust = Vec3.sum(thrust, Vec3.multiply({ x:0, y:1, z:0 }, (MyAvatar.getHeadFinalPitch() - headStartFinalPitch) * HEAD_PITCH_LIFT_THRUST * deltaTime));
            } 
        }
        //  For head trackers, adjust pitch by head pitch
        MyAvatar.headPitch += deltaPitch * HEAD_PITCH_RATE * deltaTime; 

    }
    if (isInRoom(position)) {
        // Impose constraints to keep you in the space 
        if (position.x < roomLimits.xMin) {
            thrust.x += (roomLimits.xMin - position.x) * WALL_BOUNCE * deltaTime;
        } else if (position.x > roomLimits.xMax) {
            thrust.x += (roomLimits.xMax - position.x) * WALL_BOUNCE * deltaTime;
        } 
        if (position.z < roomLimits.zMin) {
            thrust.z += (roomLimits.zMin - position.z) * WALL_BOUNCE * deltaTime;
        } else if (position.z > roomLimits.zMax) {
            thrust.z += (roomLimits.zMax - position.z) * WALL_BOUNCE * deltaTime;
        } 
    }

    // Check against movement box limits

    MyAvatar.addThrust(thrust);
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

