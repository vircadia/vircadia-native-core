"use strict";

//  webEntityLaserInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Controller, LaserPointers, RayPick, RIGHT_HAND, LEFT_HAND, Vec3, Quat, getGrabPointSphereOffset,
   makeRunningValues, Entities, NULL_UUID, enableDispatcherModule, disableDispatcherModule, makeDispatcherModuleParameters,
   PICK_MAX_DISTANCE, COLORS_GRAB_SEARCHING_HALF_SQUEEZE, COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD,
   AVATAR_SELF_ID, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE, ZERO_VEC, Overlays
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
            return Entities.keyboardFocusOverlay === laserTarget;
        }
    }

    function setKeyboardFocusOnLaserTarget(laserTarget) {
        if (laserTarget && laserTarget !== NULL_UUID) {
            Entities.wantsHandControllerPointerEvents(laserTarget);
            Overlays.keyboardFocusOverlay = NULL_UUID;
            Entities.keyboardFocusEntity = laserTarget;
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

        if (laserTarget.entityID && laserTarget.entityID !== NULL_UUID) {
            Entities.sendHoverEnterEntity(laserTarget.entityID, pointerEvent);
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

        if (laserTarget.entityID && laserTarget.entityID !== NULL_UUID) {
            Entities.sendMouseMoveOnEntity(laserTarget.entityID, pointerEvent);
            Entities.sendHoverOverEntity(laserTarget.entityID, pointerEvent);
        }
    }


    function sendTouchStartEventToLaserTarget(hand, laserTarget) {
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

        if (laserTarget.entityID && laserTarget.entityID !== NULL_UUID) {
            Entities.sendMousePressOnEntity(laserTarget.entityID, pointerEvent);
            Entities.sendClickDownOnEntity(laserTarget.entityID, pointerEvent);
        }
    }

    function sendTouchEndEventToLaserTarget(hand, laserTarget) {
        var pointerEvent = {
            type: "Release",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: laserTarget.position2D,
            pos3D: laserTarget.position,
            normal: laserTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, laserTarget.normal),
            button: "Primary"
        };

        if (laserTarget.entityID && laserTarget.entityID !== NULL_UUID) {
            Entities.sendMouseReleaseOnEntity(laserTarget.entityID, pointerEvent);
            Entities.sendClickReleaseOnEntity(laserTarget.entityID, pointerEvent);
            Entities.sendHoverLeaveEntity(laserTarget.entityID, pointerEvent);
        }
    }

    function sendTouchMoveEventToLaserTarget(hand, laserTarget) {
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

        if (laserTarget.entityID && laserTarget.entityID !== NULL_UUID) {
            Entities.sendMouseMoveOnEntity(laserTarget.entityID, pointerEvent);
            Entities.sendHoldingClickOnEntity(laserTarget.entityID, pointerEvent);
        }
    }

    function calculateTargetFromEntity(intersection, props) {
        if (props.rotation === undefined) {
            // if rotation is missing from props object, then this entity has probably been deleted.
            return null;
        }

        // project stylus tip onto entity plane.
        var normal = Vec3.multiplyQbyV(props.rotation, {x: 0, y: 0, z: 1});
        Vec3.multiplyQbyV(props.rotation, {x: 0, y: 1, z: 0});
        var distance = Vec3.dot(Vec3.subtract(intersection, props.position), normal);
        var position = Vec3.subtract(intersection, Vec3.multiply(normal, distance));

        // generate normalized coordinates
        var invRot = Quat.inverse(props.rotation);
        var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(position, props.position));
        var invDimensions = { x: 1 / props.dimensions.x, y: 1 / props.dimensions.y, z: 1 / props.dimensions.z };
        var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), props.registrationPoint);

        // 2D position on entity plane in meters, relative to the bounding box upper-left hand corner.
        var position2D = {
            x: normalizedPosition.x * props.dimensions.x,
            y: (1 - normalizedPosition.y) * props.dimensions.y // flip y-axis
        };

        return {
            entityID: props.id,
            entityProps: props,
            overlayID: null,
            distance: distance,
            position: position,
            position2D: position2D,
            normal: normal,
            normalizedPosition: normalizedPosition,
            dimensions: props.dimensions,
            valid: true
        };
    }

    function distance2D(a, b) {
        var dx = (a.x - b.x);
        var dy = (a.y - b.y);
        return Math.sqrt(dx * dx + dy * dy);
    }

    function WebEntityLaserInput(hand) {
        this.hand = hand;
        this.active = false;
        this.previousLaserClickedTarget = false;
        this.laserPressingTarget = false;
        this.hover = false;
        this.mode = "none";
        this.pressEnterLaserTarget = null;
        this.laserTarget = null;
        this.laserTargetID = null;
        this.lastValidTargetID = null;

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.getOtherModule = function() {
            return (this.hand === RIGHT_HAND) ? leftWebEntityLaserInput : rightWebEntityLaserInput;
        };

        this.parameters = makeDispatcherModuleParameters(
            550,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.requestTouchFocus = function(laserTarget) {
            if (laserTarget !== null || laserTarget !== undefined) {
                sendHoverEnterEventToLaserTarget(this.hand, this.laserTarget);
                this.lastValidTargetID = laserTarget;
            }
        };

        this.relinquishTouchFocus = function() {
            // send hover leave event.
            var pointerEvent = { type: "Move", id: this.hand + 1 };
            Entities.sendMouseMoveOnEntity(this.lastValidTargetID, pointerEvent);
            Entities.sendHoverOverEntity(this.lastValidTargetID, pointerEvent);
            Entities.sendHoverLeaveEntity(this.lastValidID, pointerEvent);
        };

        this.updateLaserTargets = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            this.laserTargetID = intersection.objectID;
            var props = Entities.getEntityProperties(intersection.objectID);
            this.laserTarget = calculateTargetFromEntity(intersection.intersection, props);
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
                    distance2D(this.laserTarget.position2D,
                        this.pressEnterLaserTarget.position2D) > this.deadspotRadius) {
                    sendTouchMoveEventToLaserTarget(this.hand, this.laserTarget);
                    this.deadspotExpired = true;
                }
            } else {
                this.laserPressingTarget = false;
            }
        };

        this.releaseTouchEvent = function() {
            if (this.pressEnterLaserTarget === null) {
                return;
            }

            sendTouchEndEventToLaserTarget(this.hand, this.pressEnterLaserTarget);
        };

        this.updateLaserPointer = function(controllerData) {
            var RADIUS = 0.005;
            var dim = { x: RADIUS, y: RADIUS, z: RADIUS };

            if (this.mode === "full") {
                fullEnd.dimensions = dim;
                LaserPointers.editRenderState(this.laserPointer, this.mode, {path: fullPath, end: fullEnd});
            } else if (this.mode === "half") {
                halfEnd.dimensions = dim;
                LaserPointers.editRenderState(this.laserPointer, this.mode, {path: halfPath, end: halfEnd});
            }

            LaserPointers.enableLaserPointer(this.laserPointer);
            LaserPointers.setRenderState(this.laserPointer, this.mode);
        };

        this.isPointingAtWebEntity = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            var entityProperty = Entities.getEntityProperties(intersection.objectID);
            var entityType = entityProperty.type;

            if ((intersection.type === RayPick.INTERSECTED_ENTITY && entityType === "Web")) {
                return true;
            }
            return false;
        };

        this.exitModule = function() {
            this.releaseTouchEvent();
            this.relinquishTouchFocus();
            this.reset();
            this.updateLaserPointer();
            LaserPointers.disableLaserPointer(this.laserPointer);
        };

        this.reset = function() {
            this.pressEnterLaserTarget = null;
            this.laserTarget = null;
            this.laserTargetID = null;
            this.laserPressingTarget = false;
            this.previousLaserClickedTarget = null;
            this.mode = "none";
            this.active = false;
        };

        this.isReady = function(controllerData) {
            var otherModule = this.getOtherModule();
            if (this.isPointingAtWebEntity(controllerData) && !otherModule.active) {
                return makeRunningValues(true, [], []);
            }

            return makeRunningValues(false, [], []);
        };

        this.run = function(controllerData, deltaTime) {
            if (!this.isPointingAtWebEntity(controllerData)) {
                this.exitModule();
                return makeRunningValues(false, [], []);
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

        this.cleanup = function() {
            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };

        this.laserPointer = LaserPointers.createLaserPointer({
            joint: (this.hand === RIGHT_HAND) ? "_CONTROLLER_RIGHTHAND" : "_CONTROLLER_LEFTHAND",
            filter: RayPick.PICK_ENTITIES,
            maxDistance: PICK_MAX_DISTANCE,
            posOffset: getGrabPointSphereOffset(this.handToController(), true),
            renderStates: renderStates,
            faceAvatar: true,
            defaultRenderStates: defaultRenderStates
        });
    }


    var leftWebEntityLaserInput = new WebEntityLaserInput(LEFT_HAND);
    var rightWebEntityLaserInput = new WebEntityLaserInput(RIGHT_HAND);

    enableDispatcherModule("LeftWebEntityLaserInput", leftWebEntityLaserInput);
    enableDispatcherModule("RightWebEntityLaserInput", rightWebEntityLaserInput);

    this.cleanup = function() {
        leftWebEntityLaserInput.cleanup();
        rightWebEntityLaserInput.cleanup();
        disableDispatcherModule("LeftWebEntityLaserInput");
        disableDispatcherModule("RightWebEntityLaserInput");
    };
    Script.scriptEnding.connect(this.cleanup);

}());
