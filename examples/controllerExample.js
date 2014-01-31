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
    print("keyPressEvent event.text=" + event.text);

    print("keyPressEvent event.isShifted=" + event.isShifted);
    print("keyPressEvent event.isControl=" + event.isControl);
    print("keyPressEvent event.isMeta=" + event.isMeta);
    print("keyPressEvent event.isAlt=" + event.isAlt);
    print("keyPressEvent event.isKeypad=" + event.isKeypad);


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
var KeyEvent_A = {
    key: "A".charCodeAt(0),
    text: "A",
    isShifted: false,
    isMeta: false
};

var KeyEvent_a = {
    text: "a",
    isShifted: false,
    isMeta: false
};

var KeyEvent_a2 = {
    key: "a".charCodeAt(0),
    isShifted: false,
    isMeta: false
};

var KeyEvent_a3 = {
    text: "a"
};

var KeyEvent_A2 = {
    text: "A"
};


var KeyEvent_9 = {
    text: "9"
};

var KeyEvent_Num = {
    text: "#"
};

var KeyEvent_At = {
    text: "@"
};

var KeyEvent_MetaAt = {
    text: "@",
    isMeta: true
};

var KeyEvent_Up = {
    text: "up"
};
var KeyEvent_Down = {
    text: "down"
};
var KeyEvent_Left = {
    text: "left"
};
var KeyEvent_Right = {
    text: "right"
};

// prevent the A key from going through to the application
print("KeyEvent_A");
Controller.captureKeyEvents(KeyEvent_A);

print("KeyEvent_A2");
Controller.captureKeyEvents(KeyEvent_A2);

print("KeyEvent_a");
Controller.captureKeyEvents(KeyEvent_a);

print("KeyEvent_a2");
Controller.captureKeyEvents(KeyEvent_a2);

print("KeyEvent_a3");
Controller.captureKeyEvents(KeyEvent_a3);

print("KeyEvent_9");
Controller.captureKeyEvents(KeyEvent_9);

print("KeyEvent_Num");
Controller.captureKeyEvents(KeyEvent_Num);

print("KeyEvent_At");
Controller.captureKeyEvents(KeyEvent_At);

print("KeyEvent_MetaAt");
Controller.captureKeyEvents(KeyEvent_MetaAt);

print("KeyEvent_Up");
Controller.captureKeyEvents(KeyEvent_Up);
print("KeyEvent_Down");
Controller.captureKeyEvents(KeyEvent_Down);
print("KeyEvent_Left");
Controller.captureKeyEvents(KeyEvent_Left);
print("KeyEvent_Right");
Controller.captureKeyEvents(KeyEvent_Right);




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
