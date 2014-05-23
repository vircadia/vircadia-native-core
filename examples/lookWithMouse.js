//
//  lookWithMouse.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Controller class
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var alwaysLook = false; // if you want the mouse look to happen only when you click, change this to false
var isMouseDown = false;
var lastX = 0;
var lastY = 0;
var yawFromMouse = 0;
var pitchFromMouse = 0;
var wantDebugging = false;

function mousePressEvent(event) {
    if (wantDebugging) {
        print("mousePressEvent event.x,y=" + event.x + ", " + event.y);
    }
    isMouseDown = true;
    lastX = event.x;
    lastY = event.y;
}

function mouseReleaseEvent(event) {
    if (wantDebugging) {
        print("mouseReleaseEvent event.x,y=" + event.x + ", " + event.y);
    }
    isMouseDown = false;
}

function mouseMoveEvent(event) {
    if (wantDebugging) {
        print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
    }

    if (alwaysLook || isMouseDown) {
        var MOUSE_YAW_SCALE = -0.25;
        var MOUSE_PITCH_SCALE = -12.5;
        var FIXED_MOUSE_TIMESTEP = 0.016;
        yawFromMouse += ((event.x - lastX) * MOUSE_YAW_SCALE * FIXED_MOUSE_TIMESTEP);
        pitchFromMouse += ((event.y - lastY) * MOUSE_PITCH_SCALE * FIXED_MOUSE_TIMESTEP);
        lastX = event.x;
        lastY = event.y;
    }
}

function update(deltaTime) {
    if (wantDebugging) {
        print("update()...");
    }
    // rotate body yaw for yaw received from mouse
    var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.fromVec3Radians( { x: 0, y: yawFromMouse, z: 0 } ));
    if (wantDebugging) {
        print("changing orientation"
            + " [old]MyAvatar.orientation="+MyAvatar.orientation.x + "," + MyAvatar.orientation.y + "," 
            + MyAvatar.orientation.z + "," + MyAvatar.orientation.w
            + " newOrientation="+newOrientation.x + "," + newOrientation.y + "," + newOrientation.z + "," + newOrientation.w);
    }
    MyAvatar.orientation = newOrientation;
    yawFromMouse = 0;

    // apply pitch from mouse
    var newPitch = MyAvatar.headPitch + pitchFromMouse;
    if (wantDebugging) {
        print("changing pitch [old]MyAvatar.headPitch="+MyAvatar.headPitch+ " newPitch="+newPitch);
    }
    MyAvatar.headPitch = newPitch;
    pitchFromMouse = 0;
}

// Map the mouse events to our functions
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

// disable the standard application for mouse events
Controller.captureMouseEvents();

function scriptEnding() {
    // re-enabled the standard application for mouse events
    Controller.releaseMouseEvents();
}

MyAvatar.bodyYaw = 0;
MyAvatar.bodyPitch = 0;
MyAvatar.bodyRoll = 0;

// would be nice to change to update
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
