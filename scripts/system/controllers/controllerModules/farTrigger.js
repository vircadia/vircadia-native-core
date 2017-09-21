"use strict";

//  farTrigger.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Controller, LaserPointers, RayPick, RIGHT_HAND, LEFT_HAND, MyAvatar, getGrabPointSphereOffset,
   makeRunningValues, Entities, enableDispatcherModule, disableDispatcherModule, makeDispatcherModuleParameters,
   PICK_MAX_DISTANCE, COLORS_GRAB_SEARCHING_HALF_SQUEEZE, COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD,
   AVATAR_SELF_ID, DEFAULT_SEARCH_SPHERE_DISTANCE, getGrabbableData
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

    function entityWantsNearTrigger(props) {
        var grabbableData = getGrabbableData(props);
        return grabbableData.triggerable || grabbableData.wantsTrigger;
    }

    function FarTriggerEntity(hand) {
        this.hand = hand;
        this.targetEntityID = null;
        this.grabbing = false;
        this.previousParentID = {};
        this.previousParentJointIndex = {};
        this.previouslyUnhooked = {};

        this.parameters = makeDispatcherModuleParameters(
            520,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.updateLaserPointer = function(controllerData) {
            var SEARCH_SPHERE_SIZE = 0.011;
            var MIN_SPHERE_SIZE = 0.0005;
            var radius = Math.max(1.2 * SEARCH_SPHERE_SIZE * this.intersectionDistance, MIN_SPHERE_SIZE);
            var dim = {x: radius, y: radius, z: radius};
            var mode = "none";
            if (controllerData.triggerClicks[this.hand]) {
                mode = "full";
            } else {
                mode = "half";
            }

            var laserPointerID = this.laserPointer;
            if (mode === "full") {
                var fullEndToEdit = this.fullEnd;
                fullEndToEdit.dimensions = dim;
                LaserPointers.editRenderState(laserPointerID, mode, {path: fullPath, end: fullEndToEdit});
            } else if (mode === "half") {
                var halfEndToEdit = this.halfEnd;
                halfEndToEdit.dimensions = dim;
                LaserPointers.editRenderState(laserPointerID, mode, {path: halfPath, end: halfEndToEdit});
            }
            LaserPointers.enableLaserPointer(laserPointerID);
            LaserPointers.setRenderState(laserPointerID, mode);
        };

        this.laserPointerOff = function() {
            LaserPointers.disableLaserPointer(this.laserPointer);
        };

        this.getTargetProps = function (controllerData) {
            // nearbyEntityProperties is already sorted by length from controller
            var targetEntity = controllerData.rayPicks[this.hand].objectID;
            if (targetEntity) {
                var targetProperties = Entities.getEntityProperties(targetEntity);
                if (entityWantsNearTrigger(targetProperties)) {
                    return targetProperties;
                }
            }
            return null;
        };

        this.startFarTrigger = function (controllerData) {
            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "startFarTrigger", args);
        };

        this.continueFarTrigger = function (controllerData) {
            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "continueFarTrigger", args);
        };

        this.endFarTrigger = function (controllerData) {
            var args = [this.hand === RIGHT_HAND ? "right" : "left", MyAvatar.sessionUUID];
            Entities.callEntityMethod(this.targetEntityID, "stopFarTrigger", args);
            this.laserPointerOff();
        };

        this.isReady = function (controllerData) {
            this.targetEntityID = null;
            if (controllerData.triggerClicks[this.hand] === 0) {
                return makeRunningValues(false, [], []);
            }

            var targetProps = this.getTargetProps(controllerData);
            if (targetProps) {
                this.targetEntityID = targetProps.id;
                this.startFarTrigger(controllerData);
                return makeRunningValues(true, [this.targetEntityID], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData) {
            var targetEntity = controllerData.rayPicks[this.hand].objectID;
            if (controllerData.triggerClicks[this.hand] === 0 || this.targetEntityID !== targetEntity) {
                this.endFarTrigger(controllerData);
                return makeRunningValues(false, [], []);
            }

            this.updateLaserPointer(controllerData);
            this.continueFarTrigger(controllerData);
            return makeRunningValues(true, [this.targetEntityID], []);
        };

        this.halfEnd = halfEnd;
        this.fullEnd = fullEnd;
        this.laserPointer = LaserPointers.createLaserPointer({
            joint: (this.hand === RIGHT_HAND) ? "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" : "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
            filter: RayPick.PICK_ENTITIES | RayPick.PICK_OVERLAYS,
            maxDistance: PICK_MAX_DISTANCE,
            posOffset: getGrabPointSphereOffset(this.handToController(), true),
            renderStates: renderStates,
            faceAvatar: true,
            defaultRenderStates: defaultRenderStates
        });

        this.cleanup = function () {
            if (this.targetEntityID) {
                this.endFarTrigger();
            }

            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };
    }

    var leftFarTriggerEntity = new FarTriggerEntity(LEFT_HAND);
    var rightFarTriggerEntity = new FarTriggerEntity(RIGHT_HAND);

    enableDispatcherModule("LeftFarTriggerEntity", leftFarTriggerEntity);
    enableDispatcherModule("RightFarTriggerEntity", rightFarTriggerEntity);

    this.cleanup = function () {
        leftFarTriggerEntity.cleanup();
        rightFarTriggerEntity.cleanup();
        disableDispatcherModule("LeftFarTriggerEntity");
        disableDispatcherModule("RightFarTriggerEntity");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
