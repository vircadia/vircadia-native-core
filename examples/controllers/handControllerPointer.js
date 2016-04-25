"use strict";
/*jslint vars: true, plusplus: true*/
/*globals Script, Overlays, Controller, Reticle, HMD, Camera, MyAvatar, Settings, Vec3, Quat, print */

//
//  handControllerPointer.js
//  examples/controllers
//
//  Created by Howard Stearns on 2016/04/22
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// For now:
// Right hand only.
// HMD only. (Desktop isn't turned off, but right now it's using
//            HMD.overlayFromWorldPoint(HMD.calculateRayUICollisionPoint ...) without compensation.)
// Cursor all the time when uncradled. (E.g., not just when blue ray is on, or five seconds after movement, etc.)
// Button 3 is left-mouse, button 4 is right-mouse.

var counter = 0, skip = 50;
function debug() { // Display the arguments not just [Object object].
    if (skip && (counter++ % skip)) { return; }
    print.apply(null, [].map.call(arguments, JSON.stringify));
}

// Keep track of the vertical fieldOfView setting:
var DEFAULT_VERTICAL_FIELD_OF_VIEW = 45; // degrees
var SETTINGS_CHANGE_RECHECK_INTERVAL = 10 * 1000; // milliseconds
var verticalFieldOfView = Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW;
var settingsChecker = Script.setInterval(function () {
    verticalFieldOfView = Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW;
}, SETTINGS_CHANGE_RECHECK_INTERVAL);
Script.scriptEnding.connect(function () { Script.clearInterval(settingsChecker); });


// Define shimmable functions for getting hand controller and setting cursor, accomodating
// the normal case of having a hand controller and the alternative at the bottom of this file.
var getControllerPose = Controller.getPoseValue;
var setCursor = Reticle.setPosition;

// Generalized HUD utilities, with or without HDM:
function calculateRayUICollisionPoint(position, direction) {
    // Answer the 3D intersection of the HUD by the given ray, or falsey if no intersection.
    if (HMD.active) {
        return HMD.calculateRayUICollisionPoint(position, direction);
    }
    // interect HUD plane, 1m in front of camera, using formula:
    //   scale = hudNormal dot (hudPoint - position) / hudNormal dot direction
    //   intersection = postion + scale*direction
    var hudNormal = Quat.getFront(Camera.getOrientation());
    var hudPoint = Vec3.sum(Camera.getPosition(), hudNormal); // 1m out
    var denominator = Vec3.dot(hudNormal, direction);
    if (denominator === 0) { return null; } // parallel to plane
    var numerator = Vec3.dot(hudNormal, Vec3.subtract(hudPoint, position));
    var scale = numerator / denominator;
    return Vec3.sum(position, Vec3.multiply(scale, direction));
}
var DEGREES_TO_HALF_RADIANS = Math.PI / 360;
function overlayFromWorldPoint(point) {
    // Answer the 2d pixel-space location in the HUD that covers the given 3D point.
    if (HMD.active) {
        return HMD.overlayFromWorldPoint(point);
    }
    var cameraToPoint = Vec3.subtract(point, Camera.getPosition());
    var cameraX = Vec3.dot(cameraToPoint, Quat.getRight(Camera.getOrientation()));
    var cameraY = Vec3.dot(cameraToPoint, Quat.getUp(Camera.getOrientation()));
    var size = Controller.getViewportDimensions();
    var hudHeight = 2 * Math.tan(verticalFieldOfView * DEGREES_TO_HALF_RADIANS);
    var hudWidth = hudHeight * size.x / size.y;
    var horizontalPixels = size.x * (cameraX / hudWidth + 0.5);
    var verticalPixels = size.y * (1 - (cameraY / hudHeight + 0.5));
    return { x: horizontalPixels, y: verticalPixels };
}

// Synthesize left and right mouse click from controller:
var MAPPING_NAME = Script.resolvePath('');
var mapping = Controller.newMapping(MAPPING_NAME);
function mapToAction(controller, button, action) {
    if (!Controller.Hardware[controller]) { return; }
    mapping.from(Controller.Hardware[controller][button]).peek().to(Controller.Actions[action]);
}
mapToAction('Hydra', 'R3', 'ReticleClick');
mapToAction('Hydra', 'R4', 'ContextMenu');
mapping.enable();
Script.scriptEnding.connect(mapping.disable);


// Here's the meat:

var LASER_COLOR = {red: 10, green: 10, blue: 255};
var terminatingBall = Overlays.addOverlay("sphere", { // Same properties as handControllerGrab search sphere
    size: 0.011,
    color: LASER_COLOR,
    ignoreRayIntersection: true,
    alpha: 0.8, // handControllerGrab has this as 0.5, but I have trouble seeing that.
    visible: true,
    solid: true
});
var laserLine = Overlays.addOverlay("line3d", { // same properties as handControllerGrab search line
    lineWidth: 5,
    color: LASER_COLOR,
    ignoreRayIntersection: true,
    visible: true,
    alpha: 1
});

function update() {
    if (Controller.getValue(Controller.Standard.RT)) { return; } // Interferes with other scripts.
    var hand = Controller.Standard.RightHand;
    var controllerPose = getControllerPose(hand);
    if (!controllerPose.valid) { return; } // Controller is cradled.
    var controllerPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, controllerPose.translation),
                                      MyAvatar.position);
    // This gets point direction right, but if you want general quaternion it would be more complicated:
    var controllerDirection = Quat.getUp(Quat.multiply(MyAvatar.orientation, controllerPose.rotation));

    var hudPoint3d = calculateRayUICollisionPoint(controllerPosition, controllerDirection);
    if (!hudPoint3d) { return; } // E.g., parallel to the screen.
    Overlays.editOverlay(terminatingBall, {position: hudPoint3d});
    Overlays.editOverlay(laserLine, {start: controllerPosition, end: hudPoint3d});
    setCursor(overlayFromWorldPoint(hudPoint3d));
}

var UPDATE_INTERVAL = 20; // milliseconds. Script.update is too frequent.
var updater = Script.setInterval(update, UPDATE_INTERVAL);
Script.scriptEnding.connect(function () {
    Overlays.deleteOverlay(terminatingBall);
    Overlays.deleteOverlay(laserLine);
    Script.clearInterval(updater);
});

// -------------------------------------------------------------------------------------------------------------------------------
// The rest of this is for debugging without working hand controllers, using a line from camera to mouse, and an image for cursor.
var CONTROLLER_ROTATION = Quat.fromPitchYawRollDegrees(90, 180, -90);
if (!Controller.Hardware.Hydra) {  // Check is made at script load time, not continuously while running.
    var mouseKeeper = {x: 0, y: 0};
    var onMouseMove = function (event) { mouseKeeper.x = event.x; mouseKeeper.y = event.y; };
    Controller.mouseMoveEvent.connect(onMouseMove);
    Script.scriptEnding.connect(function () { Controller.mouseMoveEvent.disconnect(onMouseMove); });
    getControllerPose = function () {
        var size = Controller.getViewportDimensions();
        var handPoint = Vec3.subtract(Camera.getPosition(), MyAvatar.position); // Pretend controller is at camera

        // In world-space 3D meters:
        var rotation = Camera.getOrientation();
        var normal = Quat.getFront(rotation);
        var hudHeight = 2 * Math.tan(verticalFieldOfView * DEGREES_TO_HALF_RADIANS);
        var hudWidth = hudHeight * size.x / size.y;
        var rightFraction = mouseKeeper.x / size.x - 0.5;
        var rightMeters = rightFraction * hudWidth;
        var upFraction = mouseKeeper.y / size.y - 0.5;
        var upMeters = upFraction * hudHeight * -1;
        var right = Vec3.multiply(Quat.getRight(rotation), rightMeters);
        var up = Vec3.multiply(Quat.getUp(rotation), upMeters);
        var direction = Vec3.sum(normal, Vec3.sum(right, up));
        var mouseRotation = Quat.rotationBetween(normal, direction);

        var controllerRotation = Quat.multiply(Quat.multiply(mouseRotation, rotation), CONTROLLER_ROTATION);
        var inverseAvatar = Quat.inverse(MyAvatar.orientation);
        return {
            valid: true,
            translation: Vec3.multiplyQbyV(inverseAvatar, handPoint),
            rotation: Quat.multiply(inverseAvatar, controllerRotation)
        };
    };

    // We can't set the mouse if we're using the mouse as a fake controller. So stick an image where we would be putting the mouse.
    // WARNING: This fake cursor is an overlay that will be the target of clicks and drags rather than other overlays underneath it!
    if (true) {  // Don't do the overlay, but do turn off cursor warping, which would be circular.
        setCursor = function () { };
        return;
    }
    var reticleHalfSize = 16;
    var fakeReticle = Overlays.addOverlay("image", {
        imageURL: "http://s3.amazonaws.com/hifi-public/images/delete.png",
        width: 2 * reticleHalfSize,
        height: 2 * reticleHalfSize,
        alpha: 0.7
    });
    Script.scriptEnding.connect(function () { Overlays.deleteOverlay(fakeReticle); });
    setCursor = function (hudPoint2d) {
        Overlays.editOverlay(fakeReticle, {x: hudPoint2d.x - reticleHalfSize, y: hudPoint2d.y - reticleHalfSize});
    };
}
