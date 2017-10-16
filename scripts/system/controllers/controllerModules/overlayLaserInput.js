"use strict";

//  overlayLaserInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, Controller, RIGHT_HAND, LEFT_HAND, NULL_UUID, enableDispatcherModule, disableDispatcherModule,
   makeRunningValues, Messages, Quat, Vec3, makeDispatcherModuleParameters, Overlays, ZERO_VEC, AVATAR_SELF_ID, HMD,
   INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, getGrabPointSphereOffset, COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE,
   TRIGGER_OFF_VALUE, getEnabledModuleByName, PICK_MAX_DISTANCE, LaserPointers, RayPick, ContextOverlay
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    var TouchEventUtils = Script.require("/~/system/libraries/touchEventUtils.js");
    var END_RADIUS = 0.005;
    var dim = { x: END_RADIUS, y: END_RADIUS, z: END_RADIUS };
    var halfPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };
    var halfEnd = {
        type: "sphere",
        dimensions: dim,
        solid: true,
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawInFront: true, // Even when burried inside of something, show it.
        visible: true
    };
    var fullPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };
    var fullEnd = {
        type: "sphere",
        dimensions: dim,
        solid: true,
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawInFront: true, // Even when burried inside of something, show it.
        visible: true
    };
    var holdPath = {
        type: "line3d",
        color: COLORS_GRAB_DISTANCE_HOLD,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawInFront: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };

    var renderStates = [
        {name: "half", path: halfPath, end: halfEnd},
        {name: "full", path: fullPath, end: fullEnd},
        {name: "hold", path: holdPath}
    ];

    var defaultRenderStates = [
        {name: "half", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: halfPath},
        {name: "full", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: fullPath},
        {name: "hold", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: holdPath}
    ];


    // triggered when stylus presses a web overlay/entity
    var HAPTIC_STYLUS_STRENGTH = 1.0;
    var HAPTIC_STYLUS_DURATION = 20.0;

    function distance2D(a, b) {
        var dx = (a.x - b.x);
        var dy = (a.y - b.y);
        return Math.sqrt(dx * dx + dy * dy);
    }

    function OverlayLaserInput(hand) {
        this.hand = hand;
        this.active = false;
        this.previousLaserClickedTarget = false;
        this.laserPressingTarget = false;
        this.mode = "none";
        this.laserTarget = null;
        this.pressEnterLaserTarget = null;


        this.parameters = makeDispatcherModuleParameters(
            120,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.getOtherHandController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.LeftHand : Controller.Standard.RightHand;
        };

        this.getOtherModule = function() {
            return (this.hand === RIGHT_HAND) ? leftOverlayLaserInput : rightOverlayLaserInput;
        };

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.hasTouchFocus = function(laserTarget) {
            return (laserTarget.overlayID === this.hoverOverlay);
        };

        this.requestTouchFocus = function(laserTarget) {
            if (laserTarget.overlayID &&
                laserTarget.overlayID !== this.hoverOverlay) {
                this.hoverOverlay = laserTarget.overlayID;
                TouchEventUtils.sendHoverEnterEventToTouchTarget(this.hand, laserTarget);
            }
        };

        this.relinquishTouchFocus = function() {
            // send hover leave event.
            if (this.hoverOverlay) {
                var pointerEvent = { type: "Move", id: this.hand + 1 };
                Overlays.sendMouseMoveOnOverlay(this.hoverOverlay, pointerEvent);
                Overlays.sendHoverOverOverlay(this.hoverOverlay, pointerEvent);
                Overlays.sendHoverLeaveOverlay(this.hoverOverlay, pointerEvent);
                this.hoverOverlay = null;
            }
        };

        this.relinquishStylusTargetTouchFocus = function(laserTarget) {
            var stylusModuleNames = ["LeftTabletStylusInput", "RightTabletStylusError"];
            for (var i = 0; i < stylusModuleNames.length; i++) {
                var stylusModule = getEnabledModuleByName(stylusModuleNames[i]);
                if (stylusModule) {
                    if (stylusModule.hoverOverlay === laserTarget.overlayID) {
                        stylusModule.relinquishTouchFocus();
                    }
                }
            }
        };

        this.stealTouchFocus = function(laserTarget) {
            if (laserTarget.overlayID === this.getOtherModule().hoverOverlay) {
                this.getOtherModule().relinquishTouchFocus();
            }

            // If the focus target we want to request is the same of one of the stylus
            // tell the stylus to relinquish it focus on our target
            this.relinquishStylusTargetTouchFocus(laserTarget);

            this.requestTouchFocus(laserTarget);
        };

        this.updateLaserPointer = function(controllerData) {
            LaserPointers.enableLaserPointer(this.laserPointer);
            LaserPointers.setRenderState(this.laserPointer, this.mode);
        };

        this.processControllerTriggers = function(controllerData) {
            if (controllerData.triggerClicks[this.hand]) {
                this.mode = "full";
            } else if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                this.mode = "half";
            } else {
                this.mode = "none";
            }
        };

        this.laserPressEnter = function () {
            this.stealTouchFocus(this.laserTarget);
            TouchEventUtils.sendTouchStartEventToTouchTarget(this.hand, this.laserTarget);
            Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, this.hand);

            this.touchingEnterTimer = 0;
            this.pressEnterLaserTarget = this.laserTarget;
            this.deadspotExpired = false;

            var LASER_PRESS_TO_MOVE_DEADSPOT = 0.094;
            this.deadspotRadius = Math.tan(LASER_PRESS_TO_MOVE_DEADSPOT) * this.laserTarget.distance;
        };

        this.laserPressExit = function () {
            if (this.laserTarget === null) {
                return;
            }

            // special case to handle home button.
            if (this.laserTarget.overlayID === HMD.homeButtonID) {
                Messages.sendLocalMessage("home", this.laserTarget.overlayID);
            }

            // send press event
            if (this.deadspotExpired) {
                TouchEventUtils.sendTouchEndEventToTouchTarget(this.hand, this.laserTarget);
            } else {
                TouchEventUtils.sendTouchEndEventToTouchTarget(this.hand, this.pressEnterLaserTarget);
            }
        };

        this.laserPressing = function (controllerData, dt) {
            this.touchingEnterTimer += dt;

            if (this.laserTarget) {
                if (controllerData.triggerClicks[this.hand]) {
                    var POINTER_PRESS_TO_MOVE_DELAY = 0.33; // seconds
                    if (this.deadspotExpired || this.touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY ||
                        distance2D(this.laserTarget.position2D,
                                   this.pressEnterLaserTarget.position2D) > this.deadspotRadius) {
                        TouchEventUtils.sendTouchMoveEventToTouchTarget(this.hand, this.laserTarget);
                        this.deadspotExpired = true;
                    }
                } else {
                    this.laserPressingTarget = false;
                }
            } else {
                this.laserPressingTarget = false;
            }
        };

        this.processLaser = function(controllerData) {
            if (this.shouldExit(controllerData) || this.getOtherModule().active) {
                this.exitModule();
                return false;
            }
            var intersection = controllerData.rayPicks[this.hand];
            var laserTarget = TouchEventUtils.composeTouchTargetFromIntersection(intersection);

            if (controllerData.triggerClicks[this.hand]) {
                this.laserTarget = laserTarget;
                this.laserPressingTarget = true;
            } else {
                this.requestTouchFocus(laserTarget);

                if (!TouchEventUtils.touchTargetHasKeyboardFocus(laserTarget)) {
                    TouchEventUtils.setKeyboardFocusOnTouchTarget(laserTarget);
                }

                if (this.hasTouchFocus(laserTarget) && !this.laserPressingTarget) {
                    TouchEventUtils.sendHoverOverEventToTouchTarget(this.hand, laserTarget);
                }
            }

            this.processControllerTriggers(controllerData);
            this.updateLaserPointer(controllerData);
            this.active = true;
            return true;
        };

        this.grabModuleWantsNearbyOverlay = function(controllerData) {
            if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                var nearGrabName = this.hand === RIGHT_HAND ? "RightNearParentingGrabOverlay" : "LeftNearParentingGrabOverlay";
                var nearGrabModule = getEnabledModuleByName(nearGrabName);
                if (nearGrabModule) {
                    var candidateOverlays = controllerData.nearbyOverlayIDs[this.hand];
                    var grabbableOverlays = candidateOverlays.filter(function(overlayID) {
                        return Overlays.getProperty(overlayID, "grabbable");
                    });
                    var target = nearGrabModule.getTargetID(grabbableOverlays, controllerData);
                    if (target) {
                        return true;
                    }
                }
            }
            return false;
        };

        this.shouldExit = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            var offOverlay = (intersection.type !== RayPick.INTERSECTED_OVERLAY);
            var triggerOff = (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE);
            if (triggerOff) {
                this.deleteContextOverlay();
            }
            var grabbingOverlay = this.grabModuleWantsNearbyOverlay(controllerData);
            return offOverlay || grabbingOverlay || triggerOff;
        };

        this.exitModule = function() {
            if (this.laserPressingTarget) {
                this.deadspotExpired = true;
                this.laserPressExit();
                this.laserPressingTarget = false;
            }
            this.relinquishTouchFocus();
            this.reset();
            this.updateLaserPointer();
            LaserPointers.disableLaserPointer(this.laserPointer);
        };

        this.reset = function() {
            this.mode = "none";
            this.active = false;
        };

        this.deleteContextOverlay = function() {
            var farGrabModule = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightFarActionGrabEntity" : "LeftFarActionGrabEntity");
            if (farGrabModule) {
                var entityWithContextOverlay = farGrabModule.entityWithContextOverlay;

                if (entityWithContextOverlay) {
                    ContextOverlay.destroyContextOverlay(entityWithContextOverlay);
                    farGrabModule.entityWithContextOverlay = false;
                }
            }
        };

        this.isReady = function (controllerData) {
            if (this.processLaser(controllerData)) {
                return makeRunningValues(true, [], []);
            }
            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData, deltaTime) {
            if (!this.previousLaserClickedTarget && this.laserPressingTarget) {
                this.laserPressEnter();
            }
            if (this.previousLaserClickedTarget && !this.laserPressingTarget) {
                this.laserPressExit();
            }
            this.previousLaserClickedTarget = this.laserPressingTarget;

            if (this.laserPressingTarget) {
                this.laserPressing(controllerData, deltaTime);
            }

            if (this.processLaser(controllerData)) {
                return makeRunningValues(true, [], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.cleanup = function () {
            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };

        this.laserPointer = LaserPointers.createLaserPointer({
            joint: (this.hand === RIGHT_HAND) ? "_CONTROLLER_RIGHTHAND" : "_CONTROLLER_LEFTHAND",
            filter: RayPick.PICK_OVERLAYS,
            maxDistance: PICK_MAX_DISTANCE,
            posOffset: getGrabPointSphereOffset(this.handToController(), true),
            renderStates: renderStates,
            faceAvatar: true,
            defaultRenderStates: defaultRenderStates
        });

        LaserPointers.setIgnoreOverlays(this.laserPointer, [HMD.tabletID]);
    }

    var leftOverlayLaserInput = new OverlayLaserInput(LEFT_HAND);
    var rightOverlayLaserInput = new OverlayLaserInput(RIGHT_HAND);

    enableDispatcherModule("LeftOverlayLaserInput", leftOverlayLaserInput);
    enableDispatcherModule("RightOverlayLaserInput", rightOverlayLaserInput);

    this.cleanup = function () {
        leftOverlayLaserInput.cleanup();
        rightOverlayLaserInput.cleanup();
        disableDispatcherModule("LeftOverlayLaserInput");
        disableDispatcherModule("RightOverlayLaserInput");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
