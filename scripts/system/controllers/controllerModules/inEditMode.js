"use strict";

//  inEditMode.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Controller, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, makeDispatcherModuleParameters, AVATAR_SELF_ID, HMD, getGrabPointSphereOffset, COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE,
   getEnabledModuleByName, PICK_MAX_DISTANCE, isInEditMode, LaserPointers, RayPick
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/libraries/utils.js");

(function () {
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

    function InEditMode(hand) {
        this.hand = hand;
        this.triggerClicked = false;
        this.mode = "none";

        this.parameters = makeDispatcherModuleParameters(
            160,
            this.hand === RIGHT_HAND ? ["rightHand", "rightHandEquip", "rightHandTrigger"] : ["leftHand", "leftHandEquip", "leftHandTrigger"],
            [],
            100);

        this.nearTablet = function(overlays) {
            for (var i = 0; i < overlays.length; i++) {
                if (overlays[i] === HMD.tabletID) {
                    return true;
                }
            }
            return false;
        };

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
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

        this.pointingAtTablet = function(objectID) {
            if (objectID === HMD.tabletScreenID || objectID === HMD.tabletButtonID) {
                return true;
            }
            return false;
        };

        this.sendPickData = function(controllerData) {
            if (controllerData.triggerClicks[this.hand] && !this.triggerClicked) {
                var intersection = controllerData.rayPicks[this.hand];
                if (intersection.type === RayPick.INTERSECTED_ENTITY) {
                    Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                        method: "selectEntity",
                        entityID: intersection.objectID
                    }));
                } else if (intersection.type === RayPick.INTERSECTED_OVERLAY) {
                    Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                        method: "selectOverlay",
                        overlayID: intersection.objectID
                    }));
                }

                this.triggerClicked = true;
            } else {
                this.triggerClicked = false;
            }
        };

        this.exitModule = function() {
            this.disableLasers();
            return makeRunningValues(false, [], []);
        };

        this.disableLasers = function() {
            LaserPointers.disableLaserPointer(this.laserPointer);
        };

        this.isReady = function(controllerData) {
            if (isInEditMode()) {
                this.triggerClicked = false;
                return makeRunningValues(true, [], []);
            }
            return this.exitModule();
        };

        this.run = function(controllerData) {
            var tabletStylusInput = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightTabletStylusInput" : "LeftTabletStylusInput");
            if (tabletStylusInput) {
                var tabletReady = tabletStylusInput.isReady(controllerData);

                if (tabletReady.active) {
                    return this.exitModule();
                }
            }

            var overlayLaser = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightOverlayLaserInput" : "LeftOverlayLaserInput");
            if (overlayLaser) {
                var overlayLaserReady = overlayLaser.isReady(controllerData);

                if (overlayLaserReady.active && this.pointingAtTablet(overlayLaser.target)) {
                    return this.exitModule();
                }
            }

            var nearOverlay = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightNearParentingGrabOverlay" : "LeftNearParentingGrabOverlay");
            if (nearOverlay) {
                var nearOverlayReady = nearOverlay.isReady(controllerData);

                if (nearOverlayReady.active && nearOverlay.grabbedThingID === HMD.tabletID) {
                    return this.exitModule();
                }
            }

            var teleport = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightTeleporter" : "LeftTeleporter");
            if (teleport) {
                var teleportReady = teleport.isReady(controllerData);
                if (teleportReady.active) {
                    return this.exitModule();
                }
            }

            this.processControllerTriggers(controllerData);
            this.updateLaserPointer(controllerData);
            this.sendPickData(controllerData);


            return this.isReady(controllerData);
        };

        this.cleanup = function() {
            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };


        this.halfEnd = halfEnd;
        this.fullEnd = fullEnd;

        this.laserPointer = LaserPointers.createLaserPointer({
            joint: (this.hand === RIGHT_HAND) ? "_CONTROLLER_RIGHTHAND" : "_CONTROLLER_LEFTHAND",
            filter: RayPick.PICK_ENTITIES | RayPick.PICK_OVERLAYS,
            maxDistance: PICK_MAX_DISTANCE,
            posOffset: getGrabPointSphereOffset(this.handToController(), true),
            renderStates: renderStates,
            faceAvatar: true,
            defaultRenderStates: defaultRenderStates
        });

        LaserPointers.setIgnoreOverlays(this.laserPointer, [HMD.tabletID, HMD.tabletButtonID, HMD.tabletScreenID]);
    }

    var leftHandInEditMode = new InEditMode(LEFT_HAND);
    var rightHandInEditMode = new InEditMode(RIGHT_HAND);

    enableDispatcherModule("LeftHandInEditMode", leftHandInEditMode);
    enableDispatcherModule("RightHandInEditMode", rightHandInEditMode);

    this.cleanup = function() {
        leftHandInEditMode.cleanup();
        rightHandInEditMode.cleanup();
        disableDispatcherModule("LeftHandInEditMode");
        disableDispatcherModule("RightHandInEditMode");
    };

    Script.scriptEnding.connect(this.cleanup);
}());
