"use strict";

//  farActionGrabEntity.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*jslint bitwise: true */

/* global Script, Controller, LaserPointers, RayPick, RIGHT_HAND, LEFT_HAND,
   getGrabPointSphereOffset, entityIsGrabbable,
   enableDispatcherModule, disableDispatcherModule,
   makeDispatcherModuleParameters,
   PICK_MAX_DISTANCE, COLORS_GRAB_SEARCHING_HALF_SQUEEZE, COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD,
   AVATAR_SELF_ID, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_OFF_VALUE, TRIGGER_ON_VALUE,

*/

Script.include("/~/system/controllers/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {

    var PICK_WITH_HAND_RAY = true;

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
    
    var renderStates = [{name: "half", path: halfPath, end: halfEnd},
                        {name: "full", path: fullPath, end: fullEnd},
                        {name: "hold", path: holdPath}];

    var defaultRenderStates = [{name: "half", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: halfPath},
                               {name: "full", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: fullPath},
                               {name: "hold", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: holdPath}];


    function FarActionGrabEntity(hand) {
        this.hand = hand;
        this.grabbedThingID = null;
        this.actionID = null; // action this script created...

        this.parameters = makeDispatcherModuleParameters(
            550,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.updateLaserPointer = function(controllerData, distanceHolding, distanceRotating) {
            var SEARCH_SPHERE_SIZE = 0.011;
            var MIN_SPHERE_SIZE = 0.0005;
            var radius = Math.max(1.2 * SEARCH_SPHERE_SIZE * this.intersectionDistance, MIN_SPHERE_SIZE);
            var dim = {x: radius, y: radius, z: radius};
            var mode = "hold";
            if (!distanceHolding && !distanceRotating) {
                // mode = (this.triggerSmoothedGrab() || this.secondarySqueezed()) ? "full" : "half";
                if (controllerData.triggerClicks[this.hand]
                    // || this.secondarySqueezed() // XXX
                   ) {
                    mode = "full";
                } else {
                    mode = "half";
                }
            }

            var laserPointerID = PICK_WITH_HAND_RAY ? this.laserPointer : this.headLaserPointer;
            if (mode === "full") {
                var fullEndToEdit = PICK_WITH_HAND_RAY ? this.fullEnd : fullEnd;
                fullEndToEdit.dimensions = dim;
                LaserPointers.editRenderState(laserPointerID, mode, {path: fullPath, end: fullEndToEdit});
            } else if (mode === "half") {
                var halfEndToEdit = PICK_WITH_HAND_RAY ? this.halfEnd : halfEnd;
                halfEndToEdit.dimensions = dim;
                LaserPointers.editRenderState(laserPointerID, mode, {path: halfPath, end: halfEndToEdit});
            }
            LaserPointers.enableLaserPointer(laserPointerID);
            LaserPointers.setRenderState(laserPointerID, mode);
            if (distanceHolding || distanceRotating) {
                LaserPointers.setLockEndUUID(laserPointerID, this.grabbedThingID, this.grabbedIsOverlay);
            } else {
                LaserPointers.setLockEndUUID(laserPointerID, null, false);
            }
        };

        this.laserPointerOff = function() {
            var laserPointerID = PICK_WITH_HAND_RAY ? this.laserPointer : this.headLaserPointer;
            LaserPointers.disableLaserPointer(laserPointerID);
        };


        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.isReady = function (controllerData) {
            if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                this.updateLaserPointer(controllerData, false, false);
                return true;
            } else {
                return false;
            }
        };

        this.run = function (controllerData) {
            if (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE) {
                this.laserPointerOff();
                return false;
            }


            // if we are doing a distance search and this controller moves into a position
            // where it could near-grab something, stop searching.
            var nearbyEntityProperties = controllerData.nearbyEntityProperties[this.hand];
            for (var i = 0; i < nearbyEntityProperties.length; i++) {
                var props = nearbyEntityProperties[i];
                if (entityIsGrabbable(props)) {
                    this.laserPointerOff();
                    return false;
                }
            }

            // this.updateLaserPointer(controllerData, false, false);

            // var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            // Entities.callEntityMethod(this.grabbedThingID, "continueFarGrab", args);

            return true;
        };

        this.cleanup = function () {
            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };

        this.halfEnd = halfEnd;
        this.fullEnd = fullEnd;
        this.laserPointer = LaserPointers.createLaserPointer({
            joint: (this.hand == RIGHT_HAND) ? "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" : "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
            filter: RayPick.PICK_ENTITIES | RayPick.PICK_OVERLAYS,
            maxDistance: PICK_MAX_DISTANCE,
            posOffset: getGrabPointSphereOffset(this.handToController()),
            renderStates: renderStates,
            faceAvatar: true,
            defaultRenderStates: defaultRenderStates
        });
    }

    var leftFarActionGrabEntity = new FarActionGrabEntity(LEFT_HAND);
    var rightFarActionGrabEntity = new FarActionGrabEntity(RIGHT_HAND);

    enableDispatcherModule("LeftFarActionGrabEntity", leftFarActionGrabEntity);
    enableDispatcherModule("RightFarActionGrabEntity", rightFarActionGrabEntity);

    this.cleanup = function () {
        leftFarActionGrabEntity.cleanup();
        rightFarActionGrabEntity.cleanup();
        disableDispatcherModule("LeftFarActionGrabEntity");
        disableDispatcherModule("RightFarActionGrabEntity");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
