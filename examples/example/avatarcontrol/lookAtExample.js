//
//  lookAtExample.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates use of the Camera class's lookAt(), keepLookingAt(), and stopLookingAt()
//  features.
//
//  To use the script, click on a entity, and the camera will switch into independent mode and fix it's lookAt on the point
//  on the face of the entity that you clicked. Click again and it will stop looking at that point. While in this fixed mode
//  you can use the arrow keys to change the position of the camera.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var lookingAtSomething = false;
var oldMode = Camera.mode;

function cancelLookAt() {
    if (lookingAtSomething) {
        lookingAtSomething = false;
        Camera.stopLooking();
        Camera.mode = oldMode;
        releaseMovementKeys();
    }
}

function captureMovementKeys() {
    Controller.captureKeyEvents({ text: "up" }); // Z-
    Controller.captureKeyEvents({ text: "down" }); // Z+
    Controller.captureKeyEvents({ text: "UP" }); // Y+ 
    Controller.captureKeyEvents({ text: "DOWN" }); // Y-
    Controller.captureKeyEvents({ text: "left" }); // X+
    Controller.captureKeyEvents({ text: "right" }); // X-
}

function releaseMovementKeys() {
    Controller.releaseKeyEvents({ text: "up" }); // Z-
    Controller.releaseKeyEvents({ text: "down" }); // Z+
    Controller.releaseKeyEvents({ text: "UP" }); // Y+ 
    Controller.releaseKeyEvents({ text: "DOWN" }); // Y-
    Controller.releaseKeyEvents({ text: "left" }); // X+
    Controller.releaseKeyEvents({ text: "right" }); // X-
}

var cameraPosition = Camera.getPosition();
function moveCamera(update) {
    if (lookingAtSomething) {
        Camera.setPosition(cameraPosition);
    }
}

Script.update.connect(moveCamera);


function mousePressEvent(event) {
    if (lookingAtSomething) {
        cancelLookAt();
    } else {
        var pickRay = Camera.computePickRay(event.x, event.y);
        var intersection = Entities.findRayIntersection(pickRay);
        if (intersection.intersects) {
        
            // remember the old mode we were in
            oldMode = Camera.mode;

            print("looking at intersection point: " + intersection.intersection.x + ", " 
                        + intersection.intersection.y + ", " + intersection.intersection.z);

            // switch to independent mode
            Camera.mode = "independent";

            // tell the camera to fix it's look at on the point we clicked
            Camera.keepLookingAt(intersection.intersection);

            // keep track of the fact that we're in this looking at mode
            lookingAtSomething = true;
            
            captureMovementKeys();
            cameraPosition = Camera.getPosition();
        }
    }
}
Controller.mousePressEvent.connect(mousePressEvent);

function keyPressEvent(event) {
    if (lookingAtSomething) {

        if (event.text == "ESC") {
            cancelLookAt();
        }

        var MOVE_DELTA = 0.5;

        if (event.text == "UP" && !event.isShifted) {
            cameraPosition.z -= MOVE_DELTA;
        }
        if (event.text == "DOWN" && !event.isShifted) {
            cameraPosition.z += MOVE_DELTA;
        }
        if (event.text == "LEFT" && !event.isShifted) {
            cameraPosition.x += MOVE_DELTA;
        }
        if (event.text == "RIGHT" && !event.isShifted) {
            cameraPosition.x -= MOVE_DELTA;
        }
        if (event.text == "UP" && event.isShifted) {
            cameraPosition.y += MOVE_DELTA;
        }
        if (event.text == "DOWN" && event.isShifted) {
            cameraPosition.y -= MOVE_DELTA;
        }
    }
}
Controller.keyPressEvent.connect(keyPressEvent);

function scriptEnding() {
    cancelLookAt();
}
Script.scriptEnding.connect(scriptEnding);

