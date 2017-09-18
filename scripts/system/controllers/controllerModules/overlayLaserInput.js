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

    function laserTargetHasKeyboardFocus(laserTarget) {
        if (laserTarget && laserTarget !== NULL_UUID) {
            return Overlays.keyboardFocusOverlay === laserTarget;
        }
    }

    function setKeyboardFocusOnLaserTarget(laserTarget) {
        if (laserTarget && laserTarget !== NULL_UUID) {
            Overlays.keyboardFocusOverlay = laserTarget;
            Entities.keyboardFocusEntity = NULL_UUID;
        }
    }

    function sendHoverEnterEventToLaserTarget(hand, laserTarget) {
        if (!laserTarget) {
            return;
        }
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "None"
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendHoverEnterOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    function sendHoverOverEventToLaserTarget(hand, laserTarget) {

        if (!laserTarget) {
            return;
        }
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "None"
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseMoveOnOverlay(laserTarget.overlayID, pointerEvent);
            Overlays.sendHoverOverOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchStartEventToLaserTarget(hand, laserTarget) {
        if (!laserTarget) {
            return;
        }

        var pointerEvent = {
            type: "Press",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "Primary",
            isPrimaryHeld: true
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendMousePressOnOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchEndEventToLaserTarget(hand, laserTarget) {
        if (!laserTarget) {
            return;
        }
        var pointerEvent = {
            type: "Release",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "Primary"
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseReleaseOnOverlay(laserTarget.overlayID, pointerEvent);
            Overlays.sendMouseReleaseOnOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchMoveEventToLaserTarget(hand, laserTarget) {
        if (!laserTarget) {
            return;
        }
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "Primary",
            isPrimaryHeld: true
        };

        if (laserTarget.overlayID && laserTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseReleaseOnOverlay(laserTarget.overlayID, pointerEvent);
        }
    }

    // will return undefined if overlayID does not exist.
    function calculateLaserTargetFromOverlay(worldPos, overlayID) {
        var overlayPosition = Overlays.getProperty(overlayID, "position");
        if (overlayPosition === undefined) {
            return null;
        }

        // project stylusTip onto overlay plane.
        var overlayRotation = Overlays.getProperty(overlayID, "rotation");
        if (overlayRotation === undefined) {
            return null;
        }
        var normal = Vec3.multiplyQbyV(overlayRotation, {x: 0, y: 0, z: 1});
        var distance = Vec3.dot(Vec3.subtract(worldPos, overlayPosition), normal);

        // calclulate normalized position
        var invRot = Quat.inverse(overlayRotation);
        var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(worldPos, overlayPosition));
        var dpi = Overlays.getProperty(overlayID, "dpi");

        var dimensions;
        if (dpi) {
            // Calculate physical dimensions for web3d overlay from resolution and dpi; "dimensions" property
            // is used as a scale.
            var resolution = Overlays.getProperty(overlayID, "resolution");
            if (resolution === undefined) {
                return null;
            }
            resolution.z = 1;// Circumvent divide-by-zero.
            var scale = Overlays.getProperty(overlayID, "dimensions");
            if (scale === undefined) {
                return null;
            }
            scale.z = 0.01;// overlay dimensions are 2D, not 3D.
            dimensions = Vec3.multiplyVbyV(Vec3.multiply(resolution, INCHES_TO_METERS / dpi), scale);
        } else {
            dimensions = Overlays.getProperty(overlayID, "dimensions");
            if (dimensions === undefined) {
                return null;
            }
            if (!dimensions.z) {
                dimensions.z = 0.01;// sometimes overlay dimensions are 2D, not 3D.
            }
        }
        var invDimensions = { x: 1 / dimensions.x, y: 1 / dimensions.y, z: 1 / dimensions.z };
        var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), DEFAULT_REGISTRATION_POINT);

        // 2D position on overlay plane in meters, relative to the bounding box upper-left hand corner.
        var position2D = {
            x: normalizedPosition.x * dimensions.x,
            y: (1 - normalizedPosition.y) * dimensions.y // flip y-axis
        };

        return {
            entityID: null,
            overlayID: overlayID,
            distance: distance,
            position: worldPos,
            position2D: position2D,
            normal: normal,
            normalizedPosition: normalizedPosition,
            dimensions: dimensions,
            valid: true
        };
    }

    function distance2D(a, b) {
        var dx = (a.x - b.x);
        var dy = (a.y - b.y);
        return Math.sqrt(dx * dx + dy * dy);
    }

    function OverlayLaserInput(hand) {
        this.hand = hand;
        this.active = false;
        this.previousLaserClikcedTarget = false;
        this.laserPressingTarget = false;
        this.tabletScreenID = null;
        this.mode = "none";
        this.laserTargetID = null;
        this.laserTarget = null;
        this.pressEnterLaserTarget = null;
        this.hover = false;
        this.target = null;
        this.lastValidTargetID = this.tabletTargetID;


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

        this.stealTouchFocus = function(laserTarget) {
            this.requestTouchFocus(laserTarget);
        };

        this.requestTouchFocus = function(laserTarget) {
            if (laserTarget !== null || laserTarget !== undefined) {
                sendHoverEnterEventToLaserTarget(this.hand, this.laserTarget);
                this.lastValidTargetID = laserTarget;
            }
        };

        this.relinquishTouchFocus = function() {
            // send hover leave event.
            var pointerEvent = { type: "Move", id: this.hand + 1 };
            Overlays.sendMouseMoveOnOverlay(this.lastValidTargetID, pointerEvent);
            Overlays.sendHoverOverOverlay(this.lastValidTargetID, pointerEvent);
            Overlays.sendHoverLeaveOverlay(this.lastValidID, pointerEvent);
        };

        this.updateLaserPointer = function(controllerData) {
            var RADIUS = 0.005;
            var dim = { x: RADIUS, y: RADIUS, z: RADIUS };

            if (this.mode === "full") {
                this.fullEnd.dimensions = dim;
                LaserPointers.editRenderState(this.laserPointer, this.mode, {path: fullPath, end: this.fullEnd});
            } else if (this.mode === "half") {
                this.halfEnd.dimensions = dim;
                LaserPointers.editRenderState(this.laserPointer, this.mode, {path: halfPath, end: this.halfEnd});
            }

            LaserPointers.enableLaserPointer(this.laserPointer);
            LaserPointers.setRenderState(this.laserPointer, this.mode);
        };

        this.processControllerTriggers = function(controllerData) {
            if (controllerData.triggerClicks[this.hand]) {
                this.mode = "full";
                this.laserPressingTarget = true;
                this.hover = false;
            } else if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                this.mode = "half";
                this.laserPressingTarget = false;
                this.hover = true;
                this.requestTouchFocus(this.laserTargetID);
            } else {
                this.mode = "none";
                this.laserPressingTarget = false;
                this.hover = false;
                this.relinquishTouchFocus();

            }
        };

        this.hovering = function() {
            if (!laserTargetHasKeyboardFocus(this.laserTagetID)) {
                setKeyboardFocusOnLaserTarget(this.laserTargetID);
            }
            sendHoverOverEventToLaserTarget(this.hand, this.laserTarget);
        };

        this.laserPressEnter = function () {
            sendTouchStartEventToLaserTarget(this.hand, this.laserTarget);
            Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, this.hand);

            this.touchingEnterTimer = 0;
            this.pressEnterLaserTarget = this.laserTarget;
            this.deadspotExpired = false;

            var LASER_PRESS_TO_MOVE_DEADSPOT = 0.026;
            this.deadspotRadius = Math.tan(LASER_PRESS_TO_MOVE_DEADSPOT) * this.laserTarget.distance;
        };

        this.laserPressExit = function () {
            if (this.laserTarget === null) {
                return;
            }

            // special case to handle home button.
            if (this.laserTargetID === HMD.homeButtonID) {
                Messages.sendLocalMessage("home", this.laserTargetID);
            }

            // send press event
            if (this.deadspotExpired) {
                sendTouchEndEventToLaserTarget(this.hand, this.laserTarget);
            } else {
                sendTouchEndEventToLaserTarget(this.hand, this.pressEnterLaserTarget);
            }
        };

        this.laserPressing = function (controllerData, dt) {
            this.touchingEnterTimer += dt;

            if (this.laserTarget) {
                var POINTER_PRESS_TO_MOVE_DELAY = 0.33; // seconds
                if (this.deadspotExpired || this.touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY ||
                    distance2D( this.laserTarget.position2D,
                        this.pressEnterLaserTarget.position2D) > this.deadspotRadius) {
                    sendTouchMoveEventToLaserTarget(this.hand, this.laserTarget);
                    this.deadspotExpired = true;
                }
            } else {
                this.laserPressingTarget = false;
            }
        };

        this.releaseTouchEvent = function() {
            sendTouchEndEventToLaserTarget(this.hand, this.pressEnterLaserTarget);
        };


        this.updateLaserTargets = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            this.laserTargetID = intersection.objectID;
            this.laserTarget = calculateLaserTargetFromOverlay(intersection.intersection, intersection.objectID);
        };

        this.shouldExit = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            var nearGrabName = this.hand === RIGHT_HAND ? "RightNearParentingGrabOverlay" : "LeftNearParentingGrabOverlay";
            var nearGrabModule = getEnabledModuleByName(nearGrabName);
            var status = nearGrabModule ? nearGrabModule.isReady(controllerData) : makeRunningValues(false, [], []);
            var offOverlay = (intersection.type !== RayPick.INTERSECTED_OVERLAY);
            var triggerOff = (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE);
            return offOverlay || status.active || triggerOff;
        };

        this.exitModule = function() {
            this.releaseTouchEvent();
            this.relinquishTouchFocus();
            this.reset();
            this.updateLaserPointer();
            LaserPointers.disableLaserPointer(this.laserPointer);
        };

        this.reset = function() {
            this.hover = false;
            this.pressEnterLaserTarget = null;
            this.laserTarget = null;
            this.laserTargetID = null;
            this.laserPressingTarget = false;
            this.previousLaserClickedTarget = null;
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
            this.target = null;
            var intersection = controllerData.rayPicks[this.hand];
            if (intersection.type === RayPick.INTERSECTED_OVERLAY) {
                if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE && !this.getOtherModule().active) {
                    this.target = intersection.objectID;
                    this.active = true;
                    return makeRunningValues(true, [], []);
                } else {
                    this.deleteContextOverlay();
                }
            }
            this.reset();
            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData, deltaTime) {
            if (this.shouldExit(controllerData)) {
                this.exitModule();
                return makeRunningValues(false, [], []);
            }

            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE) {
                this.deleteContextOverlay();
            }

            this.updateLaserTargets(controllerData);
            this.processControllerTriggers(controllerData);
            this.updateLaserPointer(controllerData);

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

            if (this.hover) {
                this.hovering();
            }

            return makeRunningValues(true, [], []);
        };

        this.cleanup = function () {
            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };

        this.halfEnd = halfEnd;
        this.fullEnd = fullEnd;
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
