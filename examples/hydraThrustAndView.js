//
//  hydraThrustAndView.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 2/6/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Controller and MyAvatar classes to implement
//  avatar flying through the hydra/controller joysticks
//
//

var damping = 0.9;
var position = { x: MyAvatar.position.x, y: MyAvatar.position.y, z: MyAvatar.position.z };
var joysticksCaptured = false;
var THRUST_CONTROLLER = 0;
var VIEW_CONTROLLER = 1;
var INITIAL_THRUST_MULTPLIER = 1.0;
var THRUST_INCREASE_RATE = 1.05;
var MAX_THRUST_MULTIPLIER = 75.0;
var thrustMultiplier = INITIAL_THRUST_MULTPLIER;

function flyWithHydra() {
    var deltaTime = 1/60; // approximately our FPS - maybe better to be elapsed time since last call
    var THRUST_MAG_UP = 800.0;
    var THRUST_MAG_DOWN = 300.0;
    var THRUST_MAG_FWD = 500.0;
    var THRUST_MAG_BACK = 300.0;
    var THRUST_MAG_LATERAL = 250.0;
    var THRUST_JUMP = 120.0;
    var scale = 1.0;

    var YAW_MAG = 500.0;
    var PITCH_MAG = 100.0;
    var THRUST_MAG_HAND_JETS = THRUST_MAG_FWD;
    var JOYSTICK_YAW_MAG = YAW_MAG;
    var JOYSTICK_PITCH_MAG = PITCH_MAG * 0.5;

    var thrustJoystickPosition = Controller.getJoystickPosition(THRUST_CONTROLLER);
    
    if (thrustJoystickPosition.x != 0 || thrustJoystickPosition.y != 0) {
        if (thrustMultiplier < MAX_THRUST_MULTIPLIER) {
            thrustMultiplier *= 1 + (deltaTime * THRUST_INCREASE_RATE);
        }
        var currentOrientation = MyAvatar.orientation;

        var front = Quat.getFront(currentOrientation);
        var right = Quat.getRight(currentOrientation);
        var up = Quat.getUp(currentOrientation);
    
        var thrustFront = Vec3.multiply(front, scale * THRUST_MAG_HAND_JETS * thrustJoystickPosition.y * thrustMultiplier * deltaTime);
        MyAvatar.addThrust(thrustFront);
        var thrustRight = Vec3.multiply(right, scale * THRUST_MAG_HAND_JETS * thrustJoystickPosition.x * thrustMultiplier * deltaTime);
        MyAvatar.addThrust(thrustRight);
    } else {
        thrustMultiplier = INITIAL_THRUST_MULTPLIER;
    }

    // View Controller
    var viewJoystickPosition = Controller.getJoystickPosition(VIEW_CONTROLLER);
    if (viewJoystickPosition.x != 0 || viewJoystickPosition.y != 0) {

        // change the body yaw based on our x controller
        var orientation = MyAvatar.orientation;
        var deltaOrientation = Quat.fromPitchYawRoll(0, (-1 * viewJoystickPosition.x * JOYSTICK_YAW_MAG * deltaTime), 0);
        MyAvatar.orientation = Quat.multiply(orientation, deltaOrientation);

        // change the headPitch based on our x controller
        //pitch += viewJoystickPosition.y * JOYSTICK_PITCH_MAG * deltaTime;
        var newPitch = MyAvatar.headPitch + (viewJoystickPosition.y * JOYSTICK_PITCH_MAG * deltaTime);
        MyAvatar.headPitch = newPitch;
    }
}

Script.willSendVisualDataCallback.connect(flyWithHydra);
Controller.captureJoystick(THRUST_CONTROLLER);
Controller.captureJoystick(VIEW_CONTROLLER);

// Map keyPress and mouse move events to our callbacks
function scriptEnding() {
    // re-enabled the standard application for touch events
    Controller.releaseJoystick(THRUST_CONTROLLER);
    Controller.releaseJoystick(VIEW_CONTROLLER);
}
Script.scriptEnding.connect(scriptEnding);

