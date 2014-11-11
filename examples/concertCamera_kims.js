//
//  concertCamera.js
//
//  Created by Philip Rosedale on June 24, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Move a camera through a series of pre-set locations by pressing number keys
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var oldMode;
var avatarPosition;

var cameraNumber = 0;
var freeCamera = false; 

var cameraLocations = [ {x: 8027.5, y: 237.5, z: 7305.7}, {x: 8027.5, y: 237.5, z: 7306.6}, {x: 8027.5, y: 237.5, z: 7308.0}, {x: 8027.5, y: 237.5, z: 7303.0}, {x: 8030.8, y: 238.6, z: 7311.4}, {x: 8030.9, y: 237.1, z: 7308.0} ];
var cameraLookAts = [ {x: 8027.5, y: 237.5, z: 7304.0}, {x: 8027.5, y: 237.5, z: 7305.7}, {x: 8027.5, y: 237.5, z: 7304.0}, {x: 8027.5, y: 237.5, z: 7304.0}, {x: 8027.5, y: 237.5, z: 7304.0}, {x: 8027.5, y: 237.5, z: 7304.0} ];

function saveCameraState() {
    oldMode = Camera.mode;
    avatarPosition = MyAvatar.position;
    Camera.setModeShiftPeriod(0.0);
    Camera.mode = "independent";
}

function restoreCameraState() {
    Camera.stopLooking();
    Camera.mode = oldMode;
}

function update(deltaTime) {
    if (freeCamera) { 
        var delta = Vec3.subtract(MyAvatar.position, avatarPosition);
        if (Vec3.length(delta) > 0.05) {
            cameraNumber = 0;
            freeCamera = false;
            restoreCameraState();
        }
    }
}

function keyPressEvent(event) {

    var choice = parseInt(event.text);

    if ((choice > 0) && (choice <= cameraLocations.length)) {
        print("camera " + choice);
               if (!freeCamera) {
            saveCameraState();
            freeCamera = true;
        }
        Camera.mode = "independent";
        Camera.setPosition(cameraLocations[choice - 1]);
        Camera.keepLookingAt(cameraLookAts[choice - 1]);
    }
    if (event.text == "ESC") {
        cameraNumber = 0;
        freeCamera = false;
        restoreCameraState();
    }
    if (event.text == "0") {
        //  Show camera location in log
        var cameraLocation = Camera.getPosition();
        print(cameraLocation.x + ", " + cameraLocation.y + ", " + cameraLocation.z);
    }
}

Script.update.connect(update);
Controller.keyPressEvent.connect(keyPressEvent);
