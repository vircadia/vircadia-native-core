"use strict";
/*jslint vars: true, plusplus: true*/
/*globals Script, Overlays, Controller, Reticle, HMD, Camera, Entities, MyAvatar, Settings, Menu, ScriptDiscoveryService, Window, Vec3, Quat, print*/

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

// Control the "mouse" using hand controller. (HMD and desktop.)
// First-person only.
// Starts right handed, but switches to whichever is free: Whichever hand was NOT most recently squeezed.
//   (For now, the thumb buttons on both controllers are always on.)
// When partially squeezing over a HUD element, a laser or the reticle is shown where the active hand
// controller beam intersects the HUD.


// UTILITIES -------------
//
function ignore() { }

// Utility to make it easier to setup and disconnect cleanly.
function setupHandler(event, handler) {
    event.connect(handler);
    Script.scriptEnding.connect(function () {
        event.disconnect(handler);
    });
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

function Trigger(label) {
    // This part is copied and adapted from handControllerGrab.js. Maybe we should refactor this.
    var that = this;
    that.label = label;
    that.TRIGGER_SMOOTH_RATIO = 0.1; //  Time averaging of trigger - 0.0 disables smoothing
    that.TRIGGER_OFF_VALUE = 0.10;
    that.TRIGGER_ON_VALUE = that.TRIGGER_OFF_VALUE + 0.05;     //  Squeezed just enough to activate search or near grab
    that.rawTriggerValue = 0;
    that.triggerValue = 0;           // rolling average of trigger value
    that.triggerClicked = false;
    that.triggerClick = function (value) { that.triggerClicked = value; };
    that.triggerPress = function (value) { that.rawTriggerValue = value; };
    that.updateSmoothedTrigger = function () { // e.g., call once/update for effect
        var triggerValue = that.rawTriggerValue;
        // smooth out trigger value
        that.triggerValue = (that.triggerValue * that.TRIGGER_SMOOTH_RATIO) +
            (triggerValue * (1.0 - that.TRIGGER_SMOOTH_RATIO));
        OffscreenFlags.navigationFocusDisabled = that.triggerValue != 0.0;
    };
    // Current smoothed state, without hysteresis. Answering booleans.
    that.triggerSmoothedClick = function () {
        return that.triggerClicked;
    };

    
    that.triggerSmoothedSqueezed = function () {
        return that.triggerValue > that.TRIGGER_ON_VALUE;
    };
    that.triggerSmoothedReleased = function () {
        return that.triggerValue < that.TRIGGER_OFF_VALUE;
    };

    // This part is not from handControllerGrab.js
    that.state = null; // tri-state: falsey, 'partial', 'full'
    that.update = function () { // update state, called from an update function
        var state = that.state;
        that.updateSmoothedTrigger();

        // The first two are independent of previous state:
        if (that.triggerSmoothedClick()) {
            state = 'full';
        } else if (that.triggerSmoothedReleased()) {
            state = null;
        } else if (that.triggerSmoothedSqueezed()) {
            // Another way to do this would be to have hysteresis in this branch, but that seems to make things harder to use.
            // In particular, the vive has a nice detent as you release off of full, and we want that to be a transition from
            // full to partial.
            state = 'partial';
        }
        that.state = state;
    };
    // Answer a controller source function (answering either 0.0 or 1.0).
    that.partial = function () {
        return that.state ? 1.0 : 0.0; // either 'partial' or 'full'
    };
    that.full = function () {
        return (that.state === 'full') ? 1.0 : 0.0;
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
var weMovedReticle = false;
function ignoreMouseActivity() {
    // If we're paused, or if change in cursor position is from this script, not the hardware mouse.
    if (!Reticle.allowMouseCapture) {
        return true;
    }
    var pos = Reticle.position;
    if (pos.x == -1 && pos.y == -1) {
        return true;
    }
    // Only we know if we moved it, which is why this script has to replace depthReticle.js
    if (!weMovedReticle) {
        return false;
    }
    weMovedReticle = false;
    return true;
}
var MARGIN = 25;
var reticleMinX = MARGIN, reticleMaxX, reticleMinY = MARGIN, reticleMaxY;
function updateRecommendedArea() {
    var dims = Controller.getViewportDimensions();
    reticleMaxX = dims.x - MARGIN;
    reticleMaxY = dims.y - MARGIN;
}
var setReticlePosition = function (point2d) {
    weMovedReticle = true;
    point2d.x = Math.max(reticleMinX, Math.min(point2d.x, reticleMaxX));
    point2d.y = Math.max(reticleMinY, Math.min(point2d.y, reticleMaxY));
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
// This "var" is for documentation. Do not change the value!
var PLANAR_PERPENDICULAR_HUD_DISTANCE = 1;
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
    if (denominator === 0) {
        return null;
    } // parallel to plane
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

function activeHudPoint2d(activeHand) { // if controller is valid, update reticle position and answer 2d point. Otherwise falsey.
    var controllerPose = Controller.getPoseValue(activeHand);
    // Valid if any plugged-in hand controller is "on". (uncradled Hydra, green-lighted Vive...)
    if (!controllerPose.valid) {
        return; // Controller is cradled.
    }
    var controllerPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, controllerPose.translation),
                                      MyAvatar.position);
    // This gets point direction right, but if you want general quaternion it would be more complicated:
    var controllerDirection = Quat.getUp(Quat.multiply(MyAvatar.orientation, controllerPose.rotation));

    var hudPoint3d = calculateRayUICollisionPoint(controllerPosition, controllerDirection);
    if (!hudPoint3d) {
        if (Menu.isOptionChecked("Overlays")) { // With our hud resetting strategy, hudPoint3d should be valid here
            print('Controller is parallel to HUD');  // so let us know that our assumptions are wrong.
        }
        return;
    }
    var hudPoint2d = overlayFromWorldPoint(hudPoint3d);

    // We don't know yet if we'll want to make the cursor or laser visble, but we need to move it to see if
    // it's pointing at a QML tool (aka system overlay).
    setReticlePosition(hudPoint2d);
    return hudPoint2d;
}

// MOUSE ACTIVITY --------
//
var isSeeking = false;
var averageMouseVelocity = 0, lastIntegration = 0, lastMouse;
var WEIGHTING = 1 / 20; // simple moving average over last 20 samples
var ONE_MINUS_WEIGHTING = 1 - WEIGHTING;
var AVERAGE_MOUSE_VELOCITY_FOR_SEEK_TO = 20;
function isShakingMouse() { // True if the person is waving the mouse around trying to find it.
    var now = Date.now(), mouse = Reticle.position, isShaking = false;
    if (lastIntegration && (lastIntegration !== now)) {
        var velocity = Vec3.length(Vec3.subtract(mouse, lastMouse)) / (now - lastIntegration);
        averageMouseVelocity = (ONE_MINUS_WEIGHTING * averageMouseVelocity) + (WEIGHTING * velocity);
        if (averageMouseVelocity > AVERAGE_MOUSE_VELOCITY_FOR_SEEK_TO) {
            isShaking = true;
        }
    }
    lastIntegration = now;
    lastMouse = mouse;
    return isShaking;
}
var NON_LINEAR_DIVISOR = 2;
var MINIMUM_SEEK_DISTANCE = 0.1;
function updateSeeking(doNotStartSeeking) {
    if (!doNotStartSeeking && (!Reticle.visible || isShakingMouse())) {
        if (!isSeeking) {
            print('Start seeking mouse.');
            isSeeking = true;
        }
    } // e.g., if we're about to turn it on with first movement.
    if (!isSeeking) {
        return;
    }
    averageMouseVelocity = lastIntegration = 0;
    var lookAt2D = HMD.getHUDLookAtPosition2D();
    if (!lookAt2D) { // If this happens, something has gone terribly wrong.
        print('Cannot seek without lookAt position');
        isSeeking = false;
        return; // E.g., if parallel to location in HUD
    }
    var copy = Reticle.position;
    function updateDimension(axis) {
        var distanceBetween = lookAt2D[axis] - Reticle.position[axis];
        var move = distanceBetween / NON_LINEAR_DIVISOR;
        if (Math.abs(move) < MINIMUM_SEEK_DISTANCE) {
            return false;
        }
        copy[axis] += move;
        return true;
    }
    var okX = !updateDimension('x'), okY = !updateDimension('y'); // Evaluate both. Don't short-circuit.
    if (okX && okY) {
        print('Finished seeking mouse');
        isSeeking = false;
    } else {
        Reticle.setPosition(copy); // Not setReticlePosition
    }
}

var mouseCursorActivity = new TimeLock(5000);
var APPARENT_MAXIMUM_DEPTH = 100.0; // this is a depth at which things all seem sufficiently distant
function updateMouseActivity(isClick) {
    if (ignoreMouseActivity()) {
        return;
    }
    var now = Date.now();
    mouseCursorActivity.update(now);
    if (isClick) {
        return;
    } // Bug: mouse clicks should keep going. Just not hand controller clicks
    handControllerLockOut.update(now);
    Reticle.visible = true;
}
function expireMouseCursor(now) {
    if (!isPointingAtOverlay() && mouseCursorActivity.expired(now)) {
        Reticle.visible = false;
    }
}
function hudReticleDistance() { // 3d distance from camera to the reticle position on hud
    // (The camera is only in the center of the sphere on reset.)
    var reticlePositionOnHUD = HMD.worldPointFromOverlay(Reticle.position);
    return Vec3.distance(reticlePositionOnHUD, HMD.position);
}
function onMouseMove() {
    // Display cursor at correct depth (as in depthReticle.js), and updateMouseActivity.
    if (ignoreMouseActivity()) {
        return;
    }

    if (HMD.active) { // set depth
        updateSeeking();
        if (isPointingAtOverlay()) {
            Reticle.depth = hudReticleDistance();
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
setupHandler(Controller.mouseMoveEvent, onMouseMove);
setupHandler(Controller.mousePressEvent, onMouseClick);
setupHandler(Controller.mouseDoublePressEvent, onMouseClick);

// CONTROLLER MAPPING ---------
//

var leftTrigger = new Trigger('left');
var rightTrigger = new Trigger('right');
var activeTrigger = rightTrigger;
var activeHand = Controller.Standard.RightHand;
var LEFT_HUD_LASER = 1;
var RIGHT_HUD_LASER = 2;
var BOTH_HUD_LASERS = LEFT_HUD_LASER + RIGHT_HUD_LASER;
var activeHudLaser = RIGHT_HUD_LASER;
function toggleHand() { // unequivocally switch which hand controls mouse position
    if (activeHand === Controller.Standard.RightHand) {
        activeHand = Controller.Standard.LeftHand;
        activeTrigger = leftTrigger;
        activeHudLaser = LEFT_HUD_LASER;
    } else {
        activeHand = Controller.Standard.RightHand;
        activeTrigger = rightTrigger;
        activeHudLaser = RIGHT_HUD_LASER;
    }
    clearSystemLaser();
}
function makeToggleAction(hand) { // return a function(0|1) that makes the specified hand control mouse when 1
    return function (on) {
        if (on && (activeHand !== hand)) {
            toggleHand();
        }
    };
}

var clickMapping = Controller.newMapping(Script.resolvePath('') + '-click');
Script.scriptEnding.connect(clickMapping.disable);

// Gather the trigger data for smoothing.
clickMapping.from(Controller.Standard.RT).peek().to(rightTrigger.triggerPress);
clickMapping.from(Controller.Standard.LT).peek().to(leftTrigger.triggerPress);
clickMapping.from(Controller.Standard.RTClick).peek().to(rightTrigger.triggerClick);
clickMapping.from(Controller.Standard.LTClick).peek().to(leftTrigger.triggerClick);
// Full smoothed trigger is a click.
function isPointingAtOverlayStartedNonFullTrigger(trigger) {
    // true if isPointingAtOverlay AND we were NOT full triggered when we became so.
    // The idea is to not count clicks when we're full-triggering and reach the edge of a window.
    var lockedIn = false;
    return function () {
        if (trigger !== activeTrigger) {
            return lockedIn = false;
        }
        if (!isPointingAtOverlay()) {
            return lockedIn = false;
        }
        if (lockedIn) {
            return true;
        }
        lockedIn = !trigger.full();
        return lockedIn;
    }
}
clickMapping.from(rightTrigger.full).when(isPointingAtOverlayStartedNonFullTrigger(rightTrigger)).to(Controller.Actions.ReticleClick);
clickMapping.from(leftTrigger.full).when(isPointingAtOverlayStartedNonFullTrigger(leftTrigger)).to(Controller.Actions.ReticleClick);
// The following is essentially like Left and Right versions of
// clickMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(Controller.Actions.ContextMenu);
// except that we first update the reticle position from the appropriate hand position, before invoking the ContextMenu.
var wantsMenu = 0;
clickMapping.from(function () { return wantsMenu; }).to(Controller.Actions.ContextMenu);
clickMapping.from(Controller.Standard.RightSecondaryThumb).peek().to(function (clicked) {
    if (clicked) {
        activeHudPoint2d(Controller.Standard.RightHand);
    }
    wantsMenu = clicked;
});
clickMapping.from(Controller.Standard.LeftSecondaryThumb).peek().to(function (clicked) {
    if (clicked) {
        activeHudPoint2d(Controller.Standard.LeftHand);
    }
    wantsMenu = clicked;
});
clickMapping.from(Controller.Hardware.Keyboard.RightMouseClicked).peek().to(function () {
    // Allow the reticle depth to be set correctly:
    // Wait a tick for the context menu to be displayed, and then simulate a (non-hand-controller) mouse move
    // so that the system updates qml state (Reticle.pointingAtSystemOverlay) before it gives us a mouseMove.
    // We don't want the system code to always do this for us, because, e.g., we do not want to get a mouseMove
    // after the Left/RightSecondaryThumb gives us a context menu. Only from the mouse.
    Script.setTimeout(function () {
        Reticle.setPosition(Reticle.position);
    }, 0);
});
// Partial smoothed trigger is activation.
clickMapping.from(rightTrigger.partial).to(makeToggleAction(Controller.Standard.RightHand));
clickMapping.from(leftTrigger.partial).to(makeToggleAction(Controller.Standard.LeftHand));
clickMapping.enable();

// VISUAL AID -----------
// Same properties as handControllerGrab search sphere
var LASER_ALPHA = 0.5;
var LASER_SEARCH_COLOR_XYZW = {x: 10 / 255, y: 10 / 255, z: 255 / 255, w: LASER_ALPHA};
var LASER_TRIGGER_COLOR_XYZW = {x: 250 / 255, y: 10 / 255, z: 10 / 255, w: LASER_ALPHA};
var SYSTEM_LASER_DIRECTION = {x: 0, y: 0, z: -1};
var systemLaserOn = false;
function clearSystemLaser() {
    if (!systemLaserOn) {
        return;
    }
    HMD.disableHandLasers(BOTH_HUD_LASERS);
    systemLaserOn = false;
    weMovedReticle = true;
    Reticle.position = { x: -1, y: -1 }; 
}
function setColoredLaser() { // answer trigger state if lasers supported, else falsey.
    var color = (activeTrigger.state === 'full') ? LASER_TRIGGER_COLOR_XYZW : LASER_SEARCH_COLOR_XYZW;
    return HMD.setHandLasers(activeHudLaser, true, color, SYSTEM_LASER_DIRECTION) && activeTrigger.state;
}

// MAIN OPERATIONS -----------
//
function update() {
    var now = Date.now();
    function off() {
        expireMouseCursor();
        clearSystemLaser();
    }
    updateSeeking(true);
    if (!handControllerLockOut.expired(now)) {
        return off(); // Let them use mouse in peace.
    }
    if (!Menu.isOptionChecked("First Person")) {
        return off(); // What to do? menus can be behind hand!
    }
    if (!Window.hasFocus() || !Reticle.allowMouseCapture) {
        return off(); // Don't mess with other apps or paused mouse activity
    }
    leftTrigger.update();
    rightTrigger.update();
    if (!activeTrigger.state) {
        return off(); // No trigger
    }
    var hudPoint2d = activeHudPoint2d(activeHand);
    if (!hudPoint2d) {
        return off();
    }
    // If there's a HUD element at the (newly moved) reticle, just make it visible and bail.
    if (isPointingAtOverlay(hudPoint2d)) {
        if (HMD.active) {
            Reticle.depth = hudReticleDistance();
        }
        if (activeTrigger.state && (!systemLaserOn || (systemLaserOn !== activeTrigger.state))) { // last=>wrong color
            // If the active plugin doesn't implement hand lasers, show the mouse reticle instead.
            systemLaserOn = setColoredLaser();
            Reticle.visible = !systemLaserOn;
        } else if ((systemLaserOn || Reticle.visible) && !activeTrigger.state) {
            clearSystemLaser();
            Reticle.visible = false;
        }
        return;
    }
    // We are not pointing at a HUD element (but it could be a 3d overlay).
    clearSystemLaser();
    Reticle.visible = false;
}
setupHandler(Script.update, update);

// Check periodically for changes to setup.
var SETTINGS_CHANGE_RECHECK_INTERVAL = 10 * 1000; // milliseconds
function checkSettings() {
    updateFieldOfView();
    updateRecommendedArea();
}
checkSettings();

var settingsChecker = Script.setInterval(checkSettings, SETTINGS_CHANGE_RECHECK_INTERVAL);
Script.scriptEnding.connect(function () {
    Script.clearInterval(settingsChecker);
    OffscreenFlags.navigationFocusDisabled = false;
});

