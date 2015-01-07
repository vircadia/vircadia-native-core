//
//  multitouchExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Controller class's multi-touch features
//
//  When this script is running:
//     * Four finger rotate gesture will rotate your avatar.
//     * Three finger swipe up/down will adjust the pitch of your avatars head.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var lastX = 0;
var lastY = 0;
var lastAngle = 0;
var yawFromMultiTouch = 0;
var pitchFromMultiTouch = 0;
var wantDebugging = false;
var ROTATE_YAW_SCALE = 0.15;
var MOUSE_PITCH_SCALE = -12.5;
var FIXED_MOUSE_TIMESTEP = 0.016;

var ROTATE_TOUCH_POINTS = 4;
var PITCH_TOUCH_POINTS = 3;


function printTouchEvent(eventName, event) {
    print(eventName);
    print("    event.x,y=" + event.x + ", " + event.y);
    print("    event.isPressed=" + event.isPressed);
    print("    event.isMoved=" + event.isMoved);
    print("    event.isStationary=" + event.isStationary);
    print("    event.isReleased=" + event.isReleased);
    print("    event.isShifted=" + event.isShifted);
    print("    event.isControl=" + event.isControl);
    print("    event.isMeta=" + event.isMeta);
    print("    event.isAlt=" + event.isAlt);
    print("    event.touchPoints=" + event.touchPoints);
    for (var i = 0; i < event.points.length; i++) {
        print("    event.points[" + i + "].x.y:" + event.points[i].x + ", " + event.points[i].y);
    }
    print("    event.radius=" + event.radius);
    print("    event.isPinching=" + event.isPinching);
    print("    event.isPinchOpening=" + event.isPinchOpening);

    print("    event.angle=" + event.angle);
    print("    event.deltaAngle=" + event.deltaAngle);
    for (var i = 0; i < event.points.length; i++) {
        print("    event.angles[" + i + "]:" + event.angles[i]);
    }
    print("    event.isRotating=" + event.isRotating);
    print("    event.rotating=" + event.rotating);
}

function touchBeginEvent(event) {
    printTouchEvent("touchBeginEvent", event);
    lastX = event.x;
    lastY = event.y;
}

function touchUpdateEvent(event) {
    printTouchEvent("touchUpdateEvent", event);

    if (event.isRotating && event.touchPoints == ROTATE_TOUCH_POINTS) {
        // it's possible for the multitouch rotate gesture to generate angle changes which are faster than comfortable to
        // view, so we will scale this change in angle to make it more comfortable
        var scaledRotate = event.deltaAngle * ROTATE_YAW_SCALE;
        print(">>> event.deltaAngle=" + event.deltaAngle);
        print(">>> scaledRotate=" + scaledRotate);
        yawFromMultiTouch += scaledRotate; 
    }    

    if (event.touchPoints == PITCH_TOUCH_POINTS) {
        pitchFromMultiTouch += ((event.y - lastY) * MOUSE_PITCH_SCALE * FIXED_MOUSE_TIMESTEP);
    }
    lastX = event.x;
    lastY = event.y;
    lastAngle = event.angle;
}

function touchEndEvent(event) {
    printTouchEvent("touchEndEvent", event);
}
// Map touch events to our callbacks
Controller.touchBeginEvent.connect(touchBeginEvent);
Controller.touchUpdateEvent.connect(touchUpdateEvent);
Controller.touchEndEvent.connect(touchEndEvent);



function update(deltaTime) {
    // rotate body yaw for yaw received from multitouch rotate
    var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.fromVec3Radians( { x: 0, y: yawFromMultiTouch, z: 0 } ));
    if (wantDebugging) {
        print("changing orientation"
            + " [old]MyAvatar.orientation="+MyAvatar.orientation.x + "," + MyAvatar.orientation.y + "," 
            + MyAvatar.orientation.z + "," + MyAvatar.orientation.w
            + " newOrientation="+newOrientation.x + "," + newOrientation.y + "," + newOrientation.z + "," + newOrientation.w);
    }
    MyAvatar.orientation = newOrientation;
    yawFromMultiTouch = 0;

    // apply pitch from mouse
    var newPitch = MyAvatar.headPitch + pitchFromMultiTouch;
    if (wantDebugging) {
        print("changing pitch [old]MyAvatar.headPitch="+MyAvatar.headPitch+ " newPitch="+newPitch);
    }
    MyAvatar.headPitch = newPitch;
    pitchFromMultiTouch = 0;
}
Script.update.connect(update);


function scriptEnding() {
    // handle any shutdown logic here
}

Script.scriptEnding.connect(scriptEnding);
