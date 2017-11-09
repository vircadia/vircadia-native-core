"use strict";

//  inEditMode.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Controller, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, makeDispatcherModuleParameters, HMD, getGrabPointSphereOffset, COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE,
   getEnabledModuleByName, PICK_MAX_DISTANCE, isInEditMode, LaserPointers, RayPick, Picks
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/libraries/utils.js");

(function () {
    function InEditMode(hand) {
        this.hand = hand;
        this.triggerClicked = false;

        this.parameters = makeDispatcherModuleParameters(
            160,
            this.hand === RIGHT_HAND ? ["rightHand", "rightHandEquip", "rightHandTrigger"] : ["leftHand", "leftHandEquip", "leftHandTrigger"],
            [],
            100,
            this.hand);

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

<<<<<<< HEAD
=======

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
            LaserPointers.enableLaserPointer(this.laserPointer);
            LaserPointers.setRenderState(this.laserPointer, this.mode);

            if (HMD.tabletID !== this.tabletID || HMD.homeButtonID !== this.homeButtonID || HMD.tabletScreenID !== this.tabletScreenID) {
                this.tabletID = HMD.tabletID;
                this.homeButtonID = HMD.homeButtonID;
                this.tabletScreenID = HMD.tabletScreenID;
                LaserPointers.setIgnoreItems(this.laserPointer, [HMD.tabletID, HMD.homeButtonID, HMD.tabletScreenID]);
            }
        };

>>>>>>> 5a78c9ebfee6685c32abcef652fb6b3b3f7dc348
        this.pointingAtTablet = function(objectID) {
            if (objectID === HMD.tabletScreenID || objectID === HMD.homeButtonID) {
                return true;
            }
            return false;
        };

        this.sendPickData = function(controllerData) {
            if (controllerData.triggerClicks[this.hand] && !this.triggerClicked) {
                var intersection = controllerData.rayPicks[this.hand];
                if (intersection.type === Picks.INTERSECTED_ENTITY) {
                    Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                        method: "selectEntity",
                        entityID: intersection.objectID
                    }));
                } else if (intersection.type === Picks.INTERSECTED_OVERLAY) {
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
            return makeRunningValues(false, [], []);
        };

        this.isReady = function(controllerData) {
            if (isInEditMode()) {
                this.triggerClicked = false;
                return makeRunningValues(true, [], []);
            }
            return makeRunningValues(false, [], []);
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
            this.sendPickData(controllerData);
            return this.isReady(controllerData);
        };
<<<<<<< HEAD
=======

        this.cleanup = function() {
            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };

        this.laserPointer = LaserPointers.createLaserPointer({
            joint: (this.hand === RIGHT_HAND) ? "_CONTROLLER_RIGHTHAND" : "_CONTROLLER_LEFTHAND",
            filter: Picks.PICK_ENTITIES | Picks.PICK_OVERLAYS,
            maxDistance: PICK_MAX_DISTANCE,
            posOffset: getGrabPointSphereOffset(this.handToController(), true),
            renderStates: renderStates,
            faceAvatar: true,
            defaultRenderStates: defaultRenderStates
        });

        LaserPointers.setIgnoreItems(this.laserPointer, [HMD.tabletID, HMD.homeButtonID, HMD.tabletScreenID]);
>>>>>>> 5a78c9ebfee6685c32abcef652fb6b3b3f7dc348
    }

    var leftHandInEditMode = new InEditMode(LEFT_HAND);
    var rightHandInEditMode = new InEditMode(RIGHT_HAND);

    enableDispatcherModule("LeftHandInEditMode", leftHandInEditMode);
    enableDispatcherModule("RightHandInEditMode", rightHandInEditMode);

    function cleanup() {
        leftHandInEditMode.cleanup();
        rightHandInEditMode.cleanup();
        disableDispatcherModule("LeftHandInEditMode");
        disableDispatcherModule("RightHandInEditMode");
    }

    Script.scriptEnding.connect(cleanup);
}());
