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

var cameraLocations = [ {x: 7971.9, y: 241.3, z: 7304.1}, {x: 7973.0, y: 241.3, z: 7304.1}, {x: 7975.5, y: 241.3, z: 7304.1}, {x: 7972.3, y: 241.3, z: 7303.3}, {x: 7971.0, y: 241.3, z: 7304.3}, {x: 7973.5, y: 240.7, z: 7302.5} ];
var cameraLookAts = [ {x: 7971.1, y: 241.3, z: 7304.1}, {x: 7972.1, y: 241.3, z: 7304.1}, {x: 7972.1, y: 241.3, z: 7304.1}, {x: 7972.1, y: 241.3, z: 7304.1}, {x: 7972.1, y: 241.3, z: 7304.1}, {x: 7971.3, y: 241.3, z: 7304.2} ];

function saveCameraState() {
    oldMode = Camera.getMode();
    avatarPosition = MyAvatar.position;
    Camera.setModeShiftPeriod(0.0);
    Camera.setMode("independent");
}

function restoreCameraState() {
    Camera.stopLooking();
    Camera.setMode(oldMode);
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
        Camera.setMode("independent");
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
