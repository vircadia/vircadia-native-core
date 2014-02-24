//
//  toucheVsMouse.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Controller class
//
//

function touchBeginEvent(event) {
    print("touchBeginEvent event.x,y=" + event.x + ", " + event.y);
}

function touchEndEvent(event) {
    print("touchEndEvent event.x,y=" + event.x + ", " + event.y);
}

function touchUpdateEvent(event) {
    print("touchUpdateEvent event.x,y=" + event.x + ", " + event.y);
}
function mouseMoveEvent(event) {
    print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
}

// Map the mouse events to our functions
Controller.touchBeginEvent.connect(touchBeginEvent);
Controller.touchUpdateEvent.connect(touchUpdateEvent);
Controller.touchEndEvent.connect(touchEndEvent);

Controller.mouseMoveEvent.connect(mouseMoveEvent);

function scriptEnding() {
}
Script.scriptEnding.connect(scriptEnding);
