//
//  lookWithMouse.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Controller class
//
//

var isMouseDown = false;
var lastX = 0;
var lastY = 0;
var yawFromMouse = 0;
var pitchFromMouse = 0;

function mousePressEvent(event) {
    print("mousePressEvent event.x,y=" + event.x + ", " + event.y);
    isMouseDown = true;
    lastX = event.x;
    lastY = event.y;
}

function mouseReleaseEvent(event) {
    print("mouseReleaseEvent event.x,y=" + event.x + ", " + event.y);
    isMouseDown = false;
}

function mouseMoveEvent(event) {
    print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);

    if (isMouseDown) {
        print("isMouseDown... attempting to change pitch...");
        var MOUSE_YAW_SCALE = -0.25;
        var MOUSE_PITCH_SCALE = -12.5;
        var FIXED_MOUSE_TIMESTEP = 0.016;
        yawFromMouse += ((event.x - lastX) * MOUSE_YAW_SCALE * FIXED_MOUSE_TIMESTEP);
        pitchFromMouse += ((event.y - lastY) * MOUSE_PITCH_SCALE * FIXED_MOUSE_TIMESTEP);
        lastX = event.x;
        lastY = event.y;
    }
}

function update() {
    // rotate body yaw for yaw received from mouse
    MyAvatar.orientation = Quat.multiply(MyAvatar.orientation, Quat.fromVec3( { x: 0, y: yawFromMouse, z: 0 } ));
    yawFromMouse = 0;

    // apply pitch from mouse
    MyAvatar.headPitch = MyAvatar.headPitch + pitchFromMouse;
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
Script.willSendVisualDataCallback.connect(update);
Script.scriptEnding.connect(scriptEnding);
