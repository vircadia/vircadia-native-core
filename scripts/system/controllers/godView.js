"use strict";
//
//  godView.js
//  scripts/system/
//
//  Created by Brad Hefta-Gaub on 1 Jun 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/* globals HMD, Script, Menu, Tablet, Camera */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */

(function() { // BEGIN LOCAL_SCOPE

var godView = false;

var GOD_CAMERA_OFFSET = -1; // 1 meter below the avatar
var GOD_VIEW_HEIGHT = 300; // 300 meter above the ground
var ABOVE_GROUND_DROP = 2;
var MOVE_BY = 1;

function moveTo(position) {
    if (godView) {
        MyAvatar.position = position;
        Camera.position = Vec3.sum(MyAvatar.position, {x:0, y: GOD_CAMERA_OFFSET, z: 0});
    } else {
        MyAvatar.position = position;
    }
}

function keyPressEvent(event) {
    if (godView) {
        switch(event.text) {
            case "UP":
                moveTo(Vec3.sum(MyAvatar.position, {x:0.0, y: 0, z: -1 * MOVE_BY}));
                break;
            case "DOWN":
                moveTo(Vec3.sum(MyAvatar.position, {x:0, y: 0, z: MOVE_BY}));
                break;
            case "LEFT":
                moveTo(Vec3.sum(MyAvatar.position, {x:-1 * MOVE_BY, y: 0, z: 0}));
                break;
            case "RIGHT":
                moveTo(Vec3.sum(MyAvatar.position, {x:MOVE_BY, y: 0, z: 0}));
                break;
        }
    }
}

function mousePress(event) {
    if (godView) {
        var pickRay = Camera.computePickRay(event.x, event.y);
        var pointingAt = Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction,300));
        var moveToPosition = { x: pointingAt.x, y: MyAvatar.position.y, z: pointingAt.z };
        moveTo(moveToPosition);
    }
}


var oldCameraMode = Camera.mode;

function startGodView() {
    if (!godView) {
        oldCameraMode = Camera.mode;
        MyAvatar.position = Vec3.sum(MyAvatar.position, {x:0, y: GOD_VIEW_HEIGHT, z: 0});
        Camera.mode = "independent";
        Camera.position = Vec3.sum(MyAvatar.position, {x:0, y: GOD_CAMERA_OFFSET, z: 0});
        Camera.orientation = Quat.fromPitchYawRollDegrees(-90,0,0);
        godView = true;
    }
}

function endGodView() {
    if (godView) {
        Camera.mode = oldCameraMode;
        MyAvatar.position = Vec3.sum(MyAvatar.position, {x:0, y: (-1 * GOD_VIEW_HEIGHT) + ABOVE_GROUND_DROP, z: 0});
        godView = false;
    }
}

var button;
var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

function onClicked() {
    if (godView) {
        endGodView();
    } else {
        startGodView();
    }
}

button = tablet.addButton({
    icon: "icons/tablet-icons/switch-desk-i.svg", // FIXME - consider a better icon from Alan
    text: "God View"
});

button.clicked.connect(onClicked);
Controller.keyPressEvent.connect(keyPressEvent);
Controller.mousePressEvent.connect(mousePress);


Script.scriptEnding.connect(function () {
    if (godView) {
        endGodView();
    }
    button.clicked.disconnect(onClicked);
    if (tablet) {
        tablet.removeButton(button);
    }
    Controller.keyPressEvent.disconnect(keyPressEvent);
    Controller.mousePressEvent.disconnect(mousePress);
});

}()); // END LOCAL_SCOPE
