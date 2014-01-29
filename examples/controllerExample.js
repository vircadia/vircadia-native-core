//
//  controllerExample.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates use of the Controller class
//
//

// initialize our triggers
var triggerPulled = new Array();
var numberOfTriggers = Controller.getNumberOfTriggers();
for (t = 0; t < numberOfTriggers; t++) {
    triggerPulled[t] = false;
}

function checkController() {
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    // this is expected for hydras
    if (numberOfTriggers == 2 && controllersPerTrigger == 2) {
        for (var t = 0; t < numberOfTriggers; t++) {
            var triggerValue = Controller.getTriggerValue(t);

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
}

function keyPressEvent(event) {
    print("keyPressEvent event.key=" + event.key);
    if (event.key == "A".charCodeAt(0)) {
        print("the A key was pressed");
    }
    if (event.key == " ".charCodeAt(0)) {
        print("the <space> key was pressed");
    }
}

function mouseMoveEvent(event) {
    print("mouseMoveEvent event.x,y=" + event.x + ", " + event.y);
}

function touchBeginEvent(event) {
    print("touchBeginEvent event.x,y=" + event.x + ", " + event.y);
}

function touchUpdateEvent(event) {
    print("touchUpdateEvent event.x,y=" + event.x + ", " + event.y);
}

function touchEndEvent(event) {
    print("touchEndEvent event.x,y=" + event.x + ", " + event.y);
}

// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(checkController);

// Map keyPress and mouse move events to our callbacks
Controller.keyPressEvent.connect(keyPressEvent);
var AKeyEvent = {
    key: "A".charCodeAt(0),
    isShifted: false,
    isMeta: false
};

// prevent the A key from going through to the application
Controller.captureKeyEvents(AKeyEvent);


Controller.mouseMoveEvent.connect(mouseMoveEvent);

// Map touch events to our callbacks
Controller.touchBeginEvent.connect(touchBeginEvent);
Controller.touchUpdateEvent.connect(touchUpdateEvent);
Controller.touchEndEvent.connect(touchEndEvent);

// disable the standard application for touch events
Controller.captureTouchEvents();

function scriptEnding() {
    // re-enabled the standard application for touch events
    Controller.releaseTouchEvents();
}

Agent.scriptEnding.connect(scriptEnding);
