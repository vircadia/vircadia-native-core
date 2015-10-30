//
//  controllerExample.js
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
// initialize our triggers
var triggerPulled = new Array();
var NUMBER_OF_TRIGGERS = 2;	
for (t = 0; t < NUMBER_OF_TRIGGERS; t++) {
    triggerPulled[t] = false;
}
var triggers = new Array();
triggers[0] = Controller.Standard.LT;
triggers[1] = Controller.Standard.RT;
function checkController(deltaTime) {
    var triggerToggled = false;
	for (var t = 0; t < NUMBER_OF_TRIGGERS; t++) {
            var triggerValue = Controller.getValue(triggers[t]);
            if (triggerPulled[t]) {
                // must release to at least 0.1
                if (triggerValue < 0.1) {
                    triggerPulled[t] = false; // unpulled
                }
            } else {
                // must pull to at least 0.9
                if (triggerValue > 0.9) {
                    triggerPulled[t] = true; // pulled
                    triggerToggled = true;
                }
            }
            if (triggerToggled) {
                print("a trigger was toggled");
            }
        }
	
}
// register the call back so it fires before each data send
Script.update.connect(checkController);
function printKeyEvent(eventName, event) {
    print(eventName);
    print("    event.key=" + event.key);
    print("    event.text=" + event.text);
    print("    event.isShifted=" + event.isShifted);
    print("    event.isControl=" + event.isControl);
    print("    event.isMeta=" + event.isMeta);
    print("    event.isAlt=" + event.isAlt);
    print("    event.isKeypad=" + event.isKeypad);
}
function keyPressEvent(event) {
    printKeyEvent("keyPressEvent", event);
    if (event.text == "A") {
        print("the A key was pressed");
    }
    if (event.text == " ") {
        print("the <space> key was pressed");
    }
}
function keyReleaseEvent(event) {
    printKeyEvent("keyReleaseEvent", event);
    if (event.text == "A") {
        print("the A key was released");
    }
    if (event.text == " ") {
        print("the <space> key was pressed");
    }
}
// Map keyPress and mouse move events to our callbacks
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
// prevent the A key from going through to the application
Controller.captureKeyEvents({ text: "A" });
Controller.captureKeyEvents({ key: "A".charCodeAt(0) }); // same as above, just another example of how to capture the key
Controller.captureKeyEvents({ text: " " });
Controller.captureKeyEvents({ text: "@", isMeta: true });
Controller.captureKeyEvents({ text: "page up" });
Controller.captureKeyEvents({ text: "page down" });
function printMouseEvent(eventName, event) {
    print(eventName);
    print("    event.x,y=" + event.x + ", " + event.y);
    print("    event.button=" + event.button);
    print("    event.isLeftButton=" + event.isLeftButton);
    print("    event.isRightButton=" + event.isRightButton);
    print("    event.isMiddleButton=" + event.isMiddleButton);
    print("    event.isShifted=" + event.isShifted);
    print("    event.isControl=" + event.isControl);
    print("    event.isMeta=" + event.isMeta);
    print("    event.isAlt=" + event.isAlt);
}
function mouseMoveEvent(event) {
    printMouseEvent("mouseMoveEvent", event);
}
function mousePressEvent(event) {
    printMouseEvent("mousePressEvent", event);
}
function mouseReleaseEvent(event) {
    printMouseEvent("mouseReleaseEvent", event);
}
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
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
    for (var i = 0; i < event.points.length; i++) {
        print("    event.points[" + i + "].x.y:" + event.points[i].x + ", " + event.points[i].y);
    }
    print("    event.radius=" + event.radius);
    print("    event.isPinching=" + event.isPinching);
    print("    event.isPinchOpening=" + event.isPinchOpening);
    print("    event.angle=" + event.angle);
    for (var i = 0; i < event.points.length; i++) {
        print("    event.angles[" + i + "]:" + event.angles[i]);
    }
    print("    event.isRotating=" + event.isRotating);
    print("    event.rotating=" + event.rotating);
}
function touchBeginEvent(event) {
    printTouchEvent("touchBeginEvent", event);
}
function touchUpdateEvent(event) {
    printTouchEvent("touchUpdateEvent", event);
}
function touchEndEvent(event) {
    printTouchEvent("touchEndEvent", event);
}
// Map touch events to our callbacks
Controller.touchBeginEvent.connect(touchBeginEvent);
Controller.touchUpdateEvent.connect(touchUpdateEvent);
Controller.touchEndEvent.connect(touchEndEvent);
function wheelEvent(event) {
    print("wheelEvent");
    print("    event.x,y=" + event.x + ", " + event.y);
    print("    event.delta=" + event.delta);
    print("    event.orientation=" + event.orientation);
    print("    event.isLeftButton=" + event.isLeftButton);
    print("    event.isRightButton=" + event.isRightButton);
    print("    event.isMiddleButton=" + event.isMiddleButton);
    print("    event.isShifted=" + event.isShifted);
    print("    event.isControl=" + event.isControl);
    print("    event.isMeta=" + event.isMeta);
    print("    event.isAlt=" + event.isAlt);
}
Controller.wheelEvent.connect(wheelEvent);
function scriptEnding() {
    // re-enabled the standard application for touch events
    Controller.releaseKeyEvents({ text: "A" });
    Controller.releaseKeyEvents({ key: "A".charCodeAt(0) }); // same as above, just another example of how to capture the key
    Controller.releaseKeyEvents({ text: " " });
    Controller.releaseKeyEvents({ text: "@", isMeta: true });
    Controller.releaseKeyEvents({ text: "page up" });
    Controller.releaseKeyEvents({ text: "page down" });
}
Script.scriptEnding.connect(scriptEnding);