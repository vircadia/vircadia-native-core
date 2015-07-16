//
//  cameraExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Camera class
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var damping = 0.001;
var yaw = 0.0;
var pitch = 0.0;
var roll = 0.0;
var thrust = { x: 0, y: 0, z: 0 };
var velocity = { x: 0, y: 0, z: 0 };
var position = { x: MyAvatar.position.x, y: MyAvatar.position.y + 1, z: MyAvatar.position.z };
var joysticksCaptured = false;
var THRUST_CONTROLLER = 0;
var VIEW_CONTROLLER = 1;

function checkCamera(deltaTime) {
    if (Camera.mode == "independent") {
        var THRUST_MAG_UP = 800.0;
        var THRUST_MAG_DOWN = 300.0;
        var THRUST_MAG_FWD = 500.0;
        var THRUST_MAG_BACK = 300.0;
        var THRUST_MAG_LATERAL = 250.0;
        var THRUST_JUMP = 120.0;
        var scale = 1.0;
        var thrustMultiplier = 1.0; // maybe increase this as you hold it down?

        var YAW_MAG = 500.0;
        var PITCH_MAG = 100.0;
        var THRUST_MAG_HAND_JETS = THRUST_MAG_FWD;
        var JOYSTICK_YAW_MAG = YAW_MAG;
        var JOYSTICK_PITCH_MAG = PITCH_MAG * 0.5;

        var thrustJoystickPosition = Controller.getJoystickPosition(THRUST_CONTROLLER);

        var currentOrientation = Camera.getOrientation();

        var front = Quat.getFront(currentOrientation);
        var right = Quat.getRight(currentOrientation);
        var up = Quat.getUp(currentOrientation);
        
        var thrustFront = Vec3.multiply(front, scale * THRUST_MAG_HAND_JETS * thrustJoystickPosition.y * thrustMultiplier * deltaTime);
        var thrustRight = Vec3.multiply(right, scale * THRUST_MAG_HAND_JETS * thrustJoystickPosition.x * thrustMultiplier * deltaTime);
        
        thrust = Vec3.sum(thrust, thrustFront);
        thrust = Vec3.sum(thrust, thrustRight);

        // add thrust to velocity
        velocity = Vec3.sum(velocity, Vec3.multiply(thrust, deltaTime));
        
        // add velocity to position
        position = Vec3.sum(position, Vec3.multiply(velocity, deltaTime));
        Camera.setPosition(position);
        
        // reset thrust
        thrust = { x: 0, y: 0, z: 0 };
        
        // damp velocity
        velocity = Vec3.multiply(velocity, damping);
    
        // View Controller
        var viewJoystickPosition = Controller.getJoystickPosition(VIEW_CONTROLLER);
        yaw -= viewJoystickPosition.x * JOYSTICK_YAW_MAG * deltaTime;
        pitch += viewJoystickPosition.y * JOYSTICK_PITCH_MAG * deltaTime;
        if (yaw > 360) {
            yaw -= 360;
        }
        if (yaw < -360) {
            yaw += 360;
        }
        var orientation = Quat.fromPitchYawRollDegrees(pitch, yaw, roll);
        Camera.setOrientation(orientation);
    }
}

Script.update.connect(checkCamera);

function mouseMoveEvent(event) {
    print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
    var pickRay = Camera.computePickRay(event.x, event.y);
    print("called Camera.computePickRay()");
    print("computePickRay origin=" + pickRay.origin.x + ", " + pickRay.origin.y + ", " + pickRay.origin.z);
    print("computePickRay direction=" + pickRay.direction.x + ", " + pickRay.direction.y + ", " + pickRay.direction.z);
}

Controller.mouseMoveEvent.connect(mouseMoveEvent);


function keyPressEvent(event) {
    if (joysticksCaptured) {
        Controller.releaseJoystick(THRUST_CONTROLLER);
        Controller.releaseJoystick(VIEW_CONTROLLER);
        joysticksCaptured = false;
    }

    if (event.text == "1") {
        Camera.mode = "first person";
    }

    if (event.text == "2") {
        Camera.mode = "mirror";
    }

    if (event.text == "3") {
        Camera.mode = "third person";
    }

    if (event.text == "4") {
        Camera.mode = "independent";
        joysticksCaptured = true;
        Controller.captureJoystick(THRUST_CONTROLLER);
        Controller.captureJoystick(VIEW_CONTROLLER);
        position = { x: MyAvatar.position.x, y: MyAvatar.position.y + 1, z: MyAvatar.position.z };
    }
}


// Map keyPress and mouse move events to our callbacks
Controller.keyPressEvent.connect(keyPressEvent);
Controller.captureKeyEvents({ text: "1" });
Controller.captureKeyEvents({ text: "2" });
Controller.captureKeyEvents({ text: "3" });
Controller.captureKeyEvents({ text: "4" });
function scriptEnding() {
    // re-enabled the standard application for touch events
    Controller.releaseKeyEvents({ text: "1" });
    Controller.releaseKeyEvents({ text: "2" });
    Controller.releaseKeyEvents({ text: "3" });
    Controller.releaseKeyEvents({ text: "4" });
    Controller.releaseJoystick(THRUST_CONTROLLER);
    Controller.releaseJoystick(VIEW_CONTROLLER);
}
Script.scriptEnding.connect(scriptEnding);

