//
//  multipleCursorsExample.js
//  examples
//
//  Created by Thijs Wenker on 7/23/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of multiple cursors
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var cursors = {};

function Cursor(event) {
    this.deviceID = event.deviceID;
    
    this.held_buttons = {
        LEFT: false,
        MIDDLE: false,
        RIGHT: false
    }
    this.updatePosition = function(event) {
        this.x = event.x;
        this.y = event.y;
    };
    this.pressEvent = function(event) {
        if (this.held_buttons[event.button] != undefined) {
            this.held_buttons[event.button] = true;
        }
    };
    this.releaseEvent = function(event) {
        if (this.held_buttons[event.button] != undefined) {
            this.held_buttons[event.button] = false;
        }
    };
    this.updatePosition(event);
}

function mousePressEvent(event) {
    if (cursors[event.deviceID] == undefined) {
        cursors[event.deviceID] = new Cursor(event);
    }
    cursors[event.deviceID].pressEvent(event);
}

function mouseReleaseEvent(event) {
    if (cursors[event.deviceID] == undefined) {
        cursors[event.deviceID] = new Cursor(event);
    }
    cursors[event.deviceID].releaseEvent(event);
}

function mouseMoveEvent(event) {
    if (cursors[event.deviceID] == undefined) {
        cursors[event.deviceID] = new Cursor(event);
    } else {
        cursors[event.deviceID].updatePosition(event);
    }
}

var lastOutputString = "";
function checkCursors() {
    if(lastOutputString != JSON.stringify(cursors)) {
        lastOutputString = JSON.stringify(cursors);
        // outputs json string of all cursors only when a change occured
        print(lastOutputString);
    }
}

Script.update.connect(checkCursors);

// Map the mouse events to our functions
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);