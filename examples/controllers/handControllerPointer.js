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

function debug() { // Display the arguments not just [Object object].
    print.apply(null, [].map.call(arguments, JSON.stringify));
}

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
var DEFAULT_VERTICAL_FIELD_OF_VIEW = 45; // degrees
var SETTINGS_CHANGE_RECHECK_INTERVAL = 10 * 1000; // milliseconds
var verticalHalfFieldOfView = (Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW) / 2;
var settingsChecker = Script.setInterval(function () {
    verticalHalfFieldOfView = (Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW) / 2;
}, SETTINGS_CHANGE_RECHECK_INTERVAL);
function overlayFromWorldPoint(point) {
    // Answer the 2d pixel-space location in the HUD that covers the given 3D point.
    if (HMD.active) {
        return HMD.overlayFromWorldPoint(point);
    }
    // Find the yaw and pitch from camera to position, as a fraction of view frustrum, and multiply by current screen size.
    var hudNormal = Quat.getFront(Camera.getOrientation());
    var cameraToPoint = Vec3.subtract(point, Camera.getPosition());
    var eulerDegrees = Quat.safeEulerAngles(Quat.rotationBetween(hudNormal, cameraToPoint));
    var size = Reticle.maximumPosition;
    var horizontalHalfFieldOfView = size.x * verticalHalfFieldOfView / size.y;
    return {
        x: size.x * (eulerDegrees.x / horizontalHalfFieldOfView + 0.5),
        y: size.y * (eulerDegrees.y / verticalHalfFieldOfView + 0.5)
    };
}

var MAPPING_NAME = Script.resolvePath('');
var mapping = Controller.newMapping(MAPPING_NAME);
function mapToAction(controller, button, action) {
    if (!Controller.Hardware[controller]) { return; }
    mapping.from(Controller.Hardware[controller][button]).peek().to(Controller.Actions[action]);
}
mapToAction('Hydra', 'R3', 'ReticleClick');
mapToAction('Hydra', 'R4', 'ContextMenu');
mapping.enable();

var terminatingBall = Overlays.addOverlay("sphere", { // Same properties as handControllerGrab search sphere
    size: 0.011,
    color: {red: 10, green: 10, blue: 255},
    alpha: 0.5,
    solid: true,
    visible: true
});
var counter = 0;
function update() {
    if (Controller.getValue(Controller.Standard.RT)) { return; } // Interferes with other scripts.
    var hand = Controller.Standard.RightHand,
	controllerPose = Controller.getPoseValue(hand);
    if (!controllerPose.valid) { return; } // Controller is cradled.
    var controllerPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, controllerPose.translation),
                                      MyAvatar.position);
	// This gets point direction right, but if you want general quaternion it would be more complicated:
	var controllerDirection = Quat.getUp(Quat.multiply(MyAvatar.orientation, controllerPose.rotation));
	var hudPoint3d = HMD.calculateRayUICollisionPoint(controllerPosition, controllerDirection);
    //Overlays.editOverlay(terminatingBall, {position: hudPoint3d}); // FIXME remove
    if (!hudPoint3d) { return; } // E.g., parallel to the screen.
	var hudPoint2d = HMD.overlayFromWorldPoint(hudPoint3d);
    if (!(counter++ % 50)) debug(hudPoint3d, hudPoint2d);
    Reticle.setPosition(hudPoint2d);
}

var UPDATE_INTERVAL = 20; // milliseconds. Script.update is too frequent.
var updater = Script.setInterval(update, UPDATE_INTERVAL);
Script.scriptEnding.connect(function () {
    Overlays.deleteOverlay(terminatingBall);
    Script.clearInterval(updater);
    Script.clearInterval(settingsChecker);
    mapping.disable();
});
