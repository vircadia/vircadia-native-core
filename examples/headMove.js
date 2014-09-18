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

var HEAD_MOVE_DEAD_ZONE = 0.10;
var HEAD_STRAFE_DEAD_ZONE = 0.0;
var HEAD_ROTATE_DEAD_ZONE = 0.0; 
//var HEAD_THRUST_FWD_SCALE = 12000.0;
//var HEAD_THRUST_STRAFE_SCALE = 0.0;
var HEAD_YAW_RATE = 1.0;
var HEAD_PITCH_RATE = 1.0;
//var HEAD_ROLL_THRUST_SCALE = 75.0;
//var HEAD_PITCH_LIFT_THRUST = 3.0;
var WALL_BOUNCE = 4000.0;

var HEAD_VELOCITY_FWD_FACTOR = 2.0;
var HEAD_VELOCITY_LEFT_FACTOR = 2.0;
var HEAD_VELOCITY_UP_FACTOR = 2.0;
var SHORT_TIMESCALE = 0.25;
var VERY_LARGE_TIMESCALE = 1000000.0;

var xAxis = {x:1.0, y:0.0, z:0.0 };
var yAxis = {x:0.0, y:1.0, z:0.0 };
var zAxis = {x:0.0, y:0.0, z:1.0 };

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
    var position = MyAvatar.position;
    var motorTimescale = VERY_LARGE_TIMESCALE;
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

//        var forward = Quat.getFront(Camera.getOrientation());
//        var right = Quat.getRight(Camera.getOrientation());
//        var up = Quat.getUp(Camera.getOrientation());
//        if (noFly) {
//            forward.y = 0.0;
//            forward = Vec3.normalize(forward);
//            right.y = 0.0;
//            right = Vec3.normalize(right);
//            up = { x: 0, y: 1, z: 0};
//        }

        //  Thrust based on leaning forward and side-to-side
        var targetVelocity = {x:0.0, y:0.0, z:0.0};
        if (Math.abs(headDelta.z) > HEAD_MOVE_DEAD_ZONE) {
            targetVelocity = Vec3.multiply(zAxis, -headDelta.z * HEAD_VELOCITY_FWD_FACTOR);
        }
        if (Math.abs(headDelta.x) > HEAD_STRAFE_DEAD_ZONE) {
            var deltaVelocity = Vec3.multiply(xAxis, -headDelta.x * HEAD_VELOCITY_LEFT_FACTOR);
            targetVelocity = Vec3.sum(targetVelocity, deltaVelocity);
        }
        if (Math.abs(deltaYaw) > HEAD_ROTATE_DEAD_ZONE) {
            var orientation = Quat.multiply(Quat.angleAxis((deltaYaw + deltaRoll) * HEAD_YAW_RATE * deltaTime, yAxis), MyAvatar.orientation);
            MyAvatar.orientation = orientation;
        }
        //  Thrust Up/Down based on head pitch
        if (!noFly) {
            var deltaVelocity = Vec3.multiply(yAxis, headDelta.y * HEAD_VELOCITY_UP_FACTOR);
            targetVelocity = Vec3.sum(targetVelocity, deltaVelocity);
        }
        //  For head trackers, adjust pitch by head pitch
        MyAvatar.headPitch += deltaPitch * HEAD_PITCH_RATE * deltaTime; 

        // apply the motor
        MyAvatar.setMotorVelocity(targetVelocity);
        motorTimescale = SHORT_TIMESCALE;
    }

    // Check against movement box limits
    if (isInRoom(position)) {
        var thrust = { x: 0, y: 0, z: 0 };
        // use thrust to constrain the avatar to the space 
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
        MyAvatar.addThrust(thrust);
        if (movingWithHead) {
            // reduce the timescale of the motor so that it won't defeat the thrust code
            motorTimescale = 2.0 * SHORT_TIMESCALE;
        }
    }
    MyAvatar.setMotorTimescale(motorTimescale);
}

Controller.keyPressEvent.connect(function(event) {
    if (event.text == "SPACE" && !movingWithHead) {
        movingWithHead = true;
        headStartPosition = Vec3.subtract(MyAvatar.getHeadPosition(), MyAvatar.position);
        headStartDeltaPitch = MyAvatar.getHeadDeltaPitch();
        headStartFinalPitch = MyAvatar.getHeadFinalPitch();
        headStartRoll = MyAvatar.getHeadFinalRoll();
        headStartYaw = MyAvatar.getHeadFinalYaw();
        // start with disabled motor -- it will be updated shortly
        MyAvatar.motorTimescale = VERY_LARGE_TIMESCALE;
        MyAvatar.motorVelocity = {x:0.0, y:0.0, z:0.0};
        MyAvatar.motorReferenceFrame = "camera"; // alternatives are: "avatar" and "world"
    }                        
});
Controller.keyReleaseEvent.connect(function(event) {
    if (event.text == "SPACE") {
        movingWithHead = false;
        // disable motor by giving it an obnoxiously large timescale
        MyAvatar.motorTimescale = VERY_LARGE_TIMESCALE;
        MyAvatar.motorVelocity = {x:0.0, y:0.0, z:0.0};
    }                        
});

Script.update.connect(moveWithHead);

