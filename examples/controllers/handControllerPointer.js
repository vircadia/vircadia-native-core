"use strict";
/*jslint vars: true, plusplus: true*/
/*globals Script, Overlays, Controller, Reticle, HMD, Camera, Entities, MyAvatar, Settings, Menu, ScriptDiscoveryService, Window, Vec3, Quat, print */

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

print('handControllerPointer version', 10);

// Control the "mouse" using hand controller. (HMD and desktop.)
// For now:
// Button 3 is left-mouse, button 4 is right-mouse.
// First-person only.
// Right hand only.
// On Windows, the upper left corner of Interface must be in the upper left corner of the screen, and the title bar must be 50px high. (System bug.)
// There's an additional experimental feature that isn't quite what we want:
//   A partial trigger squeeze (like "hand controller search") turns on a laser. The red ball (mostly) works as a click on in-world
//   entities and 3d overlays. We'd like that red ball to be at the end of the termination of the laser line, but right now its on the HUD.
//
// Bugs:
// While hardware mouse move switches to mouse move, hardware mouse click (without amove) does not.

var wasRunningDepthReticle = false;
function checkForDepthReticleScript() {
    ScriptDiscoveryService.getRunning().forEach(function (script) {
        if (script.name === 'depthReticle.js') {
            wasRunningDepthReticle = script.path;
            Window.alert('Shuting down depthReticle script.\n' + script.path +
                         '\nMost of the behavior is included here in\n' +
                         Script.resolvePath('') +
                        '\ndepthReticle.js will be silently restarted when this script ends.');
            ScriptDiscoveryService.stopScript(script.path);
            // FIX SYSTEM BUG: getRunning gets path and url backwards. stopScript wants a url.
            // https://app.asana.com/0/26225263936266/118428633439650
            // Some current deviations are listed below as 'FIXME'.
        }
    });
}
Script.scriptEnding.connect(function () {
    if (wasRunningDepthReticle) {
        Script.load(wasRunningDepthReticle);
    }
});


// UTILITIES -------------
//
var counter = 0, skip = 0; //fixme 50;
function debug() { // Display the arguments not just [Object object].
    if (skip && (counter++ % skip)) { return; }
    print.apply(null, [].map.call(arguments, JSON.stringify));
}

// Utility to make it easier to setup and disconnect cleanly.
function setupHandler(event, handler) {
    event.connect(handler);
    Script.scriptEnding.connect(function () { event.disconnect(handler); });
}
// If some capability is not available until expiration milliseconds after the last update.
function TimeLock(expiration) {
    var last = 0;
    this.update = function (optionalNow) {
        last = optionalNow || Date.now();
    };
    this.expired = function (optionalNow) {
        return ((optionalNow || Date.now()) - last) > expiration;
    };
}
var handControllerLockOut = new TimeLock(2000);

// Calls onFunction() or offFunction() when swtich(on), but only if it is to a new value.
function LatchedToggle(onFunction, offFunction, state) {
    this.setState = function (on) {
        if (state === on) { return; }
        state = on;
        if (on) {
            onFunction();
        } else {
            offFunction();
        }
    };
}

// Code copied and adapted from handControllerGrab.js. We should refactor this.
function Trigger() {
    var TRIGGER_SMOOTH_RATIO = 0.1; //  Time averaging of trigger - 0.0 disables smoothing
    var TRIGGER_ON_VALUE = 0.4; //  Squeezed just enough to activate search or near grab
    var TRIGGER_GRAB_VALUE = 0.85; //  Squeezed far enough to complete distant grab
    var TRIGGER_OFF_VALUE = 0.15;
    var that = this;
    that.triggerValue = 0; // rolling average of trigger value
    that.rawTriggerValue = 0;
    that.triggerPress = function(value) {
        that.rawTriggerValue = value;
    };
    that.updateSmoothedTrigger = function() {
        var triggerValue = that.rawTriggerValue;
        // smooth out trigger value
        that.triggerValue = (that.triggerValue * TRIGGER_SMOOTH_RATIO) +
            (triggerValue * (1.0 - TRIGGER_SMOOTH_RATIO));
    };
    that.triggerSmoothedGrab = function() {
        return that.triggerValue > TRIGGER_GRAB_VALUE;
    };
    that.triggerSmoothedSqueezed = function() {
        return that.triggerValue > TRIGGER_ON_VALUE;
    };
    that.triggerSmoothedReleased = function() {
        return that.triggerValue < TRIGGER_OFF_VALUE;
    };
}

// VERTICAL FIELD OF VIEW ---------
//
// Cache the verticalFieldOfView setting and update it every so often.
var verticalFieldOfView, DEFAULT_VERTICAL_FIELD_OF_VIEW = 45; // degrees
function updateFieldOfView() {
    verticalFieldOfView = Settings.getValue('fieldOfView') || DEFAULT_VERTICAL_FIELD_OF_VIEW;
}

// SHIMS ----------
//
// Define customizable versions of some standard operators. Alternative are at the bottom of the file.
var getControllerPose = Controller.getPoseValue;
var getValue = Controller.getValue;
var getOverlayAtPoint = Overlays.getOverlayAtPoint;
// FIX SYSTEM BUG: doesn't work on mac.
// https://app.asana.com/0/26225263936266/118428633439654
var setReticleVisible = function (on) { Reticle.visible = on; };

var weMovedReticle = false;
function handControllerMovedReticle() { // I.e., change in cursor position is from this script, not the mouse.
    // Only we know if we moved it, which is why this script has to replace depthReticle.js
    if (!weMovedReticle) { return false; }
    weMovedReticle = false;
    return true;
}
var setReticlePosition = function (point2d) {
    if (!HMD.active) {
        // FIX SYSTEM BUG: setPosition is setting relative to screen origin, not the content area of the window.
        // https://app.asana.com/0/26225263936266/118427643788550
        point2d = {x: point2d.x, y: point2d.y + 50};
    }
    weMovedReticle = true;
    Reticle.setPosition(point2d);
};

// Generalizations of utilities that work with system and overlay elements.
function findRayIntersection(pickRay) {
    // Check 3D overlays and entities. Argument is an object with origin and direction.
    var result = Overlays.findRayIntersection(pickRay);
    if (!result.intersects) {
        result = Entities.findRayIntersection(pickRay, true);
    }
    return result;
}
function isPointingAtOverlay(optionalHudPosition2d) {
    return Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(optionalHudPosition2d || Reticle.position);
}

// Generalized HUD utilities, with or without HMD:
// These two "vars" are for documentation. Do not change their values!
var SPHERICAL_HUD_DISTANCE = 1; // meters.
var PLANAR_PERPENDICULAR_HUD_DISTANCE = SPHERICAL_HUD_DISTANCE;
function calculateRayUICollisionPoint(position, direction) {
    // Answer the 3D intersection of the HUD by the given ray, or falsey if no intersection.
    if (HMD.active) {
        return HMD.calculateRayUICollisionPoint(position, direction);
    }
    // interect HUD plane, 1m in front of camera, using formula:
    //   scale = hudNormal dot (hudPoint - position) / hudNormal dot direction
    //   intersection = postion + scale*direction
    var hudNormal = Quat.getFront(Camera.getOrientation());
    var hudPoint = Vec3.sum(Camera.getPosition(), hudNormal); // must also scale if PLANAR_PERPENDICULAR_HUD_DISTANCE!=1
    var denominator = Vec3.dot(hudNormal, direction);
    if (denominator === 0) { return null; } // parallel to plane
    var numerator = Vec3.dot(hudNormal, Vec3.subtract(hudPoint, position));
    var scale = numerator / denominator;
    return Vec3.sum(position, Vec3.multiply(scale, direction));
}
var DEGREES_TO_HALF_RADIANS = Math.PI / 360;
function overlayFromWorldPoint(point) {
    // Answer the 2d pixel-space location in the HUD that covers the given 3D point.
    // REQUIRES: that the 3d point be on the hud surface!
    // Note that this is based on the Camera, and doesn't know anything about any
    // ray that may or may not have been used to compute the point. E.g., the
    // overlay point is NOT the intersection of some non-camera ray with the HUD.
    if (HMD.active) {
        return HMD.overlayFromWorldPoint(point);
    }
    var cameraToPoint = Vec3.subtract(point, Camera.getPosition());
    var cameraX = Vec3.dot(cameraToPoint, Quat.getRight(Camera.getOrientation()));
    var cameraY = Vec3.dot(cameraToPoint, Quat.getUp(Camera.getOrientation()));
    var size = Controller.getViewportDimensions();
    var hudHeight = 2 * Math.tan(verticalFieldOfView * DEGREES_TO_HALF_RADIANS); // must adjust if PLANAR_PERPENDICULAR_HUD_DISTANCE!=1
    var hudWidth = hudHeight * size.x / size.y;
    var horizontalFraction = (cameraX / hudWidth + 0.5);
    var verticalFraction = 1 - (cameraY / hudHeight + 0.5);
    var horizontalPixels = size.x * horizontalFraction;
    var verticalPixels = size.y * verticalFraction;
    return { x: horizontalPixels, y: verticalPixels };
}

// MOUSE ACTIVITY --------
//
var mouseCursorActivity = new TimeLock(5000);
var APPARENT_MAXIMUM_DEPTH = 100.0; // this is a depth at which things all seem sufficiently distant
function updateMouseActivity(isClick) {
    if (handControllerMovedReticle()) { return; }
    var now = Date.now();
    mouseCursorActivity.update(now);
    if (isClick) { return; } // Bug: mouse clicks should keep going. Just not hand controller clicks
    // FIXME: Does not yet seek to lookAt upon waking.
    // FIXME not unless Reticle.allowMouseCapture
    handControllerLockOut.update(now);
    setReticleVisible(true);
}
function expireMouseCursor(now) {
    if (!isPointingAtOverlay() && mouseCursorActivity.expired(now)) {
        setReticleVisible(false);
    }
}
function onMouseMove() {
    // Display cursor at correct depth (as in depthReticle.js), and updateMouseActivity.
    if (handControllerMovedReticle()) { return; }

    if (HMD.active) { // set depth
        // FIXME: does not yet adjust slowly.
        if (isPointingAtOverlay()) {
            Reticle.depth = SPHERICAL_HUD_DISTANCE; // NOT CORRECT IF WE SWITCH TO OFFSET SPHERE!
        } else {
            var result = findRayIntersection(Camera.computePickRay(Reticle.position.x, Reticle.position.y));
            Reticle.depth = result.intersects ? result.distance : APPARENT_MAXIMUM_DEPTH;
        }
    }
    updateMouseActivity(); // After the above, just in case the depth movement is awkward when becoming visible.
}
function onMouseClick() {
    updateMouseActivity(true);
}
var fixmehack = false;
if (!fixmehack) {
    setupHandler(Controller.mouseMoveEvent, onMouseMove);
    setupHandler(Controller.mousePressEvent, onMouseClick);
    setupHandler(Controller.mouseDoublePressEvent, onMouseClick);
}

// CONTROLLER MAPPING ---------
//
// Synthesize left and right mouse click from controller, and get trigger values matching handControllerGrab.
function mapToAction(mapping, controller, button, action) {
    if (!Controller.Hardware[controller]) { return; } // FIXME: recheck periodically!
    mapping.from(Controller.Hardware[controller][button]).peek().to(Controller.Actions[action]);
}

var triggerMenuMapping = Controller.newMapping(Script.resolvePath('') + '-trigger');
Script.scriptEnding.connect(triggerMenuMapping.disable);
var leftTrigger = new Trigger();
var rightTrigger = new Trigger();
triggerMenuMapping.from([Controller.Standard.RT]).peek().to(rightTrigger.triggerPress);
triggerMenuMapping.from([Controller.Standard.LT]).peek().to(leftTrigger.triggerPress);
mapToAction(triggerMenuMapping, 'Hydra', 'R4', 'ContextMenu');
mapToAction(triggerMenuMapping, 'Hydra', 'L4', 'ContextMenu');
triggerMenuMapping.enable();

var clickMapping = Controller.newMapping(Script.resolvePath('') + '-click');
Script.scriptEnding.connect(clickMapping.disable);
mapToAction(clickMapping, 'Hydra', 'R3', 'ReticleClick');
mapToAction(clickMapping, 'Hydra', 'L3', 'ReticleClick');
mapToAction(clickMapping, 'Vive', 'LeftPrimaryThumb', 'ReticleClick');
mapToAction(clickMapping, 'Vive', 'RightPrimaryThumb', 'ReticleClick');
var clickMapToggle = new LatchedToggle(clickMapping.enable, clickMapping.disable);
clickMapToggle.setState(true);


// VISUAL AID -----------
var LASER_COLOR = {red: 10, green: 10, blue: 255};
var laserLine = Overlays.addOverlay("line3d", { // same properties as handControllerGrab search line
    lineWidth: 5,
    // FIX SYSTEM BUG?: If you don't supply a start and end at creation, it will never show up, even after editing.
    start: MyAvatar.position,
    end: Vec3.ZERO,
    color: LASER_COLOR,
    ignoreRayIntersection: true,
    visible: false,
    alpha: 1
});
var BALL_SIZE = 0.011;
var BALL_ALPHA = 0.5;
var laserBall = Overlays.addOverlay("sphere", { // Same properties as handControllerGrab search sphere
    size: BALL_SIZE,
    color: LASER_COLOR,
    ignoreRayIntersection: true,
    alpha: BALL_ALPHA,
    visible: false,
    solid: true,
    drawInFront: true // Even when burried inside of something, show it.
});
var fakeProjectionBall = Overlays.addOverlay("sphere", { // Same properties as handControllerGrab search sphere
    size: 5 * BALL_SIZE,
    color: {red: 255, green: 10, blue: 10},
    ignoreRayIntersection: true,
    alpha: BALL_ALPHA,
    visible: false,
    solid: true,
    drawInFront: true // Even when burried inside of something, show it.
});
var overlays = [laserBall, laserLine, fakeProjectionBall];
Script.scriptEnding.connect(function () { overlays.forEach(Overlays.deleteOverlay); });
var visualizationIsShowing = false; // Not whether it desired, but simply whether it is. Just an optimziation.
var wantsVisualization = false;
function turnOffLaser(optionalEnableClicks) {
    if (!optionalEnableClicks) {
        expireMouseCursor();
        wantsVisualization = false;
    }
    if (!visualizationIsShowing) { return; }
    visualizationIsShowing = false;
    //toggleMap.setState(optionalEnableClicks);
    overlays.forEach(function (overlay) {
        Overlays.editOverlay(overlay, {visible: false});
    });
}
var MAX_RAY_SCALE = 32000; // Anything large. It's a scale, not a distance.
function updateLaser(controllerPosition, controllerDirection, hudPosition3d, hudPosition2d) {
    if (!wantsVisualization) { return false; }
    // Show the laser and intersect it with 3d overlays and entities.
    function intersection3d(position, direction) {
        var pickRay = {origin: position, direction: direction};
        var result = findRayIntersection(pickRay);
        return result.intersects ? result.intersection : Vec3.sum(position, Vec3.multiply(MAX_RAY_SCALE, direction));
    }
    var termination = intersection3d(controllerPosition, controllerDirection);

    visualizationIsShowing = true;
    Overlays.editOverlay(laserLine, {visible: true, start: controllerPosition, end: termination});
    // We show the ball at the hud intersection rather than at the termination because:
    // 1) As you swing the laser in space, it's hard to judge where it will intersect with a HUD element,
    //    unless the intersection of the laser with the HUD is marked. But it's confusing to do that
    //    with the pointer, so we use the ball.
    // 2) On some objects, the intersection is just enough inside the object that we're not going to see
    //    the ball anyway.
    Overlays.editOverlay(laserBall, {visible: true, position: hudPosition3d});

    if (!fixmehack) {
        // We really want in-world interactions to take place at termination:
        //   - We could do some of that with callEntityMethod (e.g., light switch entity script)
        //   - But we would have to alter edit.js to accept synthetic mouse data.
        // So for now, we present a false projection of the cursor onto whatever is below it. This is different from
        // the laser termination because the false projection is from the camera, while the laser termination is from the hand.
        var eye = Camera.getPosition();
        var falseProjection = intersection3d(eye, Vec3.subtract(hudPosition3d, eye));
        Overlays.editOverlay(fakeProjectionBall, {visible: true, position: falseProjection});
        setReticleVisible(false);
    } else {
        // Hack: Move the pointer again, this time to the intersection. This allows "clicking" on
        // 3D entities without rewriting other parts of the system, but it isn't right,
        // because the line from camera to the new mouse position might intersect different things
        // than the line from controllerPosition to termination.
        var eye = Camera.getPosition();
        var apparentHudTermination3d = calculateRayUICollisionPoint(eye, Vec3.subtract(termination, eye));
        var apparentHudTermination2d = overlayFromWorldPoint(apparentHudTermination3d);
        Overlays.editOverlay(fakeProjectionBall, {visible: true, position: termination});
        setReticleVisible(false);

        setReticlePosition(apparentHudTermination2d);
        /*if (isPointingAtOverlay(apparentHudTermination2d)) {
            // The intersection could be at a point underneath a HUD element! (I said this was a hack.)
            // If so, set things back so we don't oscillate.
            setReticleVisible(false);
            setReticlePosition(hudPosition2d)
            return true;
        } else {
            setReticleVisible(true);
        }*/
        if (HMD.active) { Reticle.depth = Vec3.distance(eye, apparentHudTermination3d); }
    }

    return true;
}

// MAIN OPERATIONS -----------
//
function update() {
    var now = Date.now();
    rightTrigger.updateSmoothedTrigger();
    if (!handControllerLockOut.expired(now)) { return turnOffLaser(); } // Let them use mouse it in peace.
    if (!Menu.isOptionChecked("First Person")) { return turnOffLaser(); }  // What to do? menus can be behind hand!
    if (rightTrigger.triggerSmoothedGrab()) { handControllerLockOut.update(now); return turnOffLaser(); } // Interferes with other scripts.
    if (rightTrigger.triggerSmoothedSqueezed()) { wantsVisualization = true; }
    var hand = Controller.Standard.RightHand;
    var controllerPose = getControllerPose(hand);
    if (!controllerPose.valid) { return turnOffLaser(); } // Controller is cradled.
    var controllerPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, controllerPose.translation),
                                      MyAvatar.position);
    // This gets point direction right, but if you want general quaternion it would be more complicated:
    var controllerDirection = Quat.getUp(Quat.multiply(MyAvatar.orientation, controllerPose.rotation));

    var hudPoint3d = calculateRayUICollisionPoint(controllerPosition, controllerDirection);
    if (!hudPoint3d) { print('Controller is parallel to HUD'); return turnOffLaser(); }
    var hudPoint2d = overlayFromWorldPoint(hudPoint3d);

    // We don't know yet if we'll want to make the cursor visble, but we need to move it to see if
    // it's pointing at a QML tool (aka system overlay).
    setReticlePosition(hudPoint2d);
    // If there's a HUD element at the (newly moved) reticle, just make it visible and bail.
    if (isPointingAtOverlay(hudPoint2d)) {
        setReticleVisible(true);
        Reticle.depth = SPHERICAL_HUD_DISTANCE; // NOT CORRECT IF WE SWITCH TO OFFSET SPHERE!
        clickMapToggle.setState(true);
        return turnOffLaser(true);
    }
    // We are not pointing at a HUD element (but it could be a 3d overlay).
    var visualization3d = updateLaser(controllerPosition, controllerDirection, hudPoint3d, hudPoint2d);
    clickMapToggle.setState(visualization3d);
    if (!visualization3d) {
        setReticleVisible(false);
        turnOffLaser();
    }
}

var UPDATE_INTERVAL = 20; // milliseconds. Script.update is too frequent.
var updater = Script.setInterval(update, UPDATE_INTERVAL);
Script.scriptEnding.connect(function () { Script.clearInterval(updater); });

// Check periodically for changes to setup.
var SETTINGS_CHANGE_RECHECK_INTERVAL = 10 * 1000; // milliseconds
function checkSettings() {
    updateFieldOfView();
    checkForDepthReticleScript()    
}
checkSettings();
var settingsChecker = Script.setInterval(checkSettings, SETTINGS_CHANGE_RECHECK_INTERVAL);
Script.scriptEnding.connect(function () { Script.clearInterval(settingsChecker); });


// DEBUGGING WITHOUT HYDRA -----------------------
//
// The rest of this is for debugging without working hand controllers, using a line from camera to mouse, and an image for cursor.
var CONTROLLER_ROTATION = Quat.fromPitchYawRollDegrees(90, 180, -90);
if (false && !Controller.Hardware.Hydra) {
    print('WARNING: no hand controller detected. Using mouse!');
    var mouseKeeper = {x: 0, y: 0};
    var onMouseMoveCapture = function (event) { mouseKeeper.x = event.x; mouseKeeper.y = event.y; };
    setupHandler(Controller.mouseMoveEvent, onMouseMoveCapture);
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
    var reticleHalfSize = 16;
    var fakeReticle = Overlays.addOverlay("image", {
        imageURL: "http://s3.amazonaws.com/hifi-public/images/delete.png",
        width: 2 * reticleHalfSize,
        height: 2 * reticleHalfSize,
        alpha: 0.7
    });
    Script.scriptEnding.connect(function () { Overlays.deleteOverlay(fakeReticle); });
    setReticlePosition = function (hudPoint2d) {
        weMovedReticle = true;
        Overlays.editOverlay(fakeReticle, {x: hudPoint2d.x - reticleHalfSize, y: hudPoint2d.y - reticleHalfSize});
    };
    setReticleVisible = function (on) {
        Reticle.visible = on;
        Overlays.editOverlay(fakeReticle, {visible: on});
    };
    // The idea here is that we not return a truthy result constantly when we display the fake reticle.
    // But this is done wrong when we're over another overlay as well: if we hit the fakeReticle, we incorrectly answer null here.
    // FIXME: display fake reticle slightly off to the side instead.
    getOverlayAtPoint = function (point2d) {
        var overlay = Overlays.getOverlayAtPoint(point2d);
        if (overlay === fakeReticle) { return null; }
        return overlay;
    };
    var fakeTrigger = 0;
    getValue = function () { var trigger = fakeTrigger; fakeTrigger = 0; return trigger; };
    setupHandler(Controller.keyPressEvent, function (event) {
        switch (event.text) {
        case '`':
            fakeTrigger = 0.4;
            break;
        case '~':
            fakeTrigger = 0.9;
            break;
        }
    });
}
