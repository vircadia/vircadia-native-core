//
//  look.js
//  examples
//
//  Copyright 2015 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Controller class
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var wantDebugging = false;


// Configuration
var TOUCH_YAW_SCALE = -0.25;
var TOUCH_PITCH_SCALE = -12.5;
var FIXED_TOUCH_TIMESTEP = 0.016;

var MOUSE_YAW_SCALE = -0.25;
var MOUSE_PITCH_SCALE = -12.5;
var FIXED_MOUSE_TIMESTEP = 0.016;

// Mouse Data
var alwaysLook = false; // if you want the mouse look to happen only when you click, change this to false
var isMouseDown = false;
var lastTouchX = 0;
var lastTouchY = 0;
var yawFromTouch = 0;
var pitchFromTouch = 0;

// Touch Data
var TIME_BEFORE_GENERATED_END_TOUCH_EVENT = 50; // ms
var startedTouching = false;
var lastTouchEvent = 0;
var lastMouseX = 0;
var lastMouseY = 0;
var yawFromMouse = 0;
var pitchFromMouse = 0;


// Mouse Events
function mousePressEvent(event) {
    if (wantDebugging) {
        print("mousePressEvent event.x,y=" + event.x + ", " + event.y);
    }
    if (event.isRightButton) {
        isMouseDown = true;
        lastMouseX = event.x;
        lastMouseY = event.y;
    }
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
        yawFromMouse += ((event.x - lastMouseX) * MOUSE_YAW_SCALE * FIXED_MOUSE_TIMESTEP);
        pitchFromMouse += ((event.y - lastMouseY) * MOUSE_PITCH_SCALE * FIXED_MOUSE_TIMESTEP);
        lastMouseX = event.x;
        lastMouseY = event.y;
    }
}

// Touch Events
function touchBeginEvent(event) {
    if (wantDebugging) {
        print("touchBeginEvent event.x,y=" + event.x + ", " + event.y);
    }
    lastTouchX = event.x;
    lastTouchY = event.y;
    yawFromTouch = 0;
    pitchFromTouch = 0;
    startedTouching = true;
    var d = new Date();
    lastTouchEvent = d.getTime();
}

function touchEndEvent(event) {
    if (wantDebugging) {
        if (event) {
            print("touchEndEvent event.x,y=" + event.x + ", " + event.y);
        } else {
            print("touchEndEvent generated");
        }
    }
    startedTouching = false;
}

function touchUpdateEvent(event) {
    // print("TOUCH UPDATE");
    if (wantDebugging) {
        print("touchUpdateEvent event.x,y=" + event.x + ", " + event.y);
    }

    if (!startedTouching) {
        // handle Qt 5.4.x bug where we get touch update without a touch begin event
        touchBeginEvent(event);
        return;
    }

    yawFromTouch += ((event.x - lastTouchX) * TOUCH_YAW_SCALE * FIXED_TOUCH_TIMESTEP);
    pitchFromTouch += ((event.y - lastTouchY) * TOUCH_PITCH_SCALE * FIXED_TOUCH_TIMESTEP);
    lastTouchX = event.x;
    lastTouchY = event.y;
    var d = new Date();
    lastTouchEvent = d.getTime();
}


function update(deltaTime) {
    if (wantDebugging) {
        print("update()...");
    }
    
    if (startedTouching) {
        var d = new Date();
        var sinceLastTouch = d.getTime() - lastTouchEvent;
        if (sinceLastTouch > TIME_BEFORE_GENERATED_END_TOUCH_EVENT) {
            touchEndEvent();
        }
    }

    if (yawFromTouch != 0 || yawFromMouse != 0) {
        var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.fromPitchYawRollRadians(0, yawFromTouch + yawFromMouse, 0));

        if (wantDebugging) {
            print("changing orientation"
                + " [old]MyAvatar.orientation="+MyAvatar.orientation.x + "," + MyAvatar.orientation.y + ","
                + MyAvatar.orientation.z + "," + MyAvatar.orientation.w
                + " newOrientation="+newOrientation.x + "," + newOrientation.y + "," + newOrientation.z + "," + newOrientation.w);
        }

        MyAvatar.orientation = newOrientation;
        yawFromTouch = 0;
        yawFromMouse = 0;
    }

    if (pitchFromTouch != 0 || pitchFromMouse != 0) {
        var newPitch = MyAvatar.headPitch + pitchFromTouch + pitchFromMouse;

        if (wantDebugging) {
            print("changing pitch [old]MyAvatar.headPitch="+MyAvatar.headPitch+ " newPitch="+newPitch);
        }

        MyAvatar.headPitch = newPitch;
        pitchFromTouch = 0;
        pitchFromMouse = 0;
    }
}


// Map the mouse events to our functions
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);

// Map the mouse events to our functions
Controller.touchBeginEvent.connect(touchBeginEvent);
Controller.touchUpdateEvent.connect(touchUpdateEvent);
Controller.touchEndEvent.connect(touchEndEvent);

// disable the standard application for mouse events
Controller.captureTouchEvents();

function scriptEnding() {
    // re-enabled the standard application for mouse events
    Controller.releaseTouchEvents();
}

// would be nice to change to update
Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
