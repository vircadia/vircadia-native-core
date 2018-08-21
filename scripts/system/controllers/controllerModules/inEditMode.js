"use strict";

//  inEditMode.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Controller, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, makeDispatcherModuleParameters, HMD, getGrabPointSphereOffset, COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE,
   getEnabledModuleByName, PICK_MAX_DISTANCE, isInEditMode, Picks, makeLaserParams, Entities
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/libraries/utils.js");

(function () {
    var MARGIN = 25;
    function InEditMode(hand) {
        this.hand = hand;
        this.triggerClicked = false;
        this.selectedTarget = null;
        this.reticleMinX = MARGIN;
        this.reticleMaxX;
        this.reticleMinY = MARGIN;
        this.reticleMaxY;

        this.parameters = makeDispatcherModuleParameters(
            200,
            this.hand === RIGHT_HAND ? ["rightHand", "rightHandEquip", "rightHandTrigger"] : ["leftHand", "leftHandEquip", "leftHandTrigger"],
            [],
            100,
            makeLaserParams(this.hand, false));

        this.nearTablet = function(overlays) {
            for (var i = 0; i < overlays.length; i++) {
                if (HMD.tabletID && overlays[i] === HMD.tabletID) {
                    return true;
                }
            }
            return false;
        };

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.pointingAtTablet = function(objectID) {
            return (HMD.tabletScreenID && objectID === HMD.tabletScreenID)
                || (HMD.homeButtonID && objectID === HMD.homeButtonID);
        };

        this.calculateNewReticlePosition = function(intersection) {
            var dims = Controller.getViewportDimensions();
            this.reticleMaxX = dims.x - MARGIN;
            this.reticleMaxY = dims.y - MARGIN;
            var point2d = HMD.overlayFromWorldPoint(intersection);
            point2d.x = Math.max(this.reticleMinX, Math.min(point2d.x, this.reticleMaxX));
            point2d.y = Math.max(this.reticleMinY, Math.min(point2d.y, this.reticleMaxY));
            return point2d;
        };

        this.sendPickData = function(controllerData) {
            if (controllerData.triggerClicks[this.hand]) {
                var hand = this.hand === RIGHT_HAND ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
                if (!this.triggerClicked) {
                    this.selectedTarget = controllerData.rayPicks[this.hand];
                    if (!this.selectedTarget.intersects) {
                        Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                            method: "clearSelection",
                            hand: hand
                        }));
                    }
                }
                if (this.selectedTarget.type === Picks.INTERSECTED_ENTITY) {
                    Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                        method: "selectEntity",
                        entityID: this.selectedTarget.objectID,
                        hand: hand
                    }));
                } else if (this.selectedTarget.type === Picks.INTERSECTED_OVERLAY) {
                    Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                        method: "selectOverlay",
                        overlayID: this.selectedTarget.objectID,
                        hand: hand
                    }));
                }

                this.triggerClicked = true;
            }

            this.sendPointingAtData(controllerData);
        };

        this.sendPointingAtData = function(controllerData) {
            var rayPick = controllerData.rayPicks[this.hand];
            var hudRayPick = controllerData.hudRayPicks[this.hand];
            var point2d = this.calculateNewReticlePosition(hudRayPick.intersection);
            var desktopWindow = Window.isPointOnDesktopWindow(point2d);
            var tablet = this.pointingAtTablet(rayPick.objectID);
            var rightHand = this.hand === RIGHT_HAND;
            Messages.sendLocalMessage("entityToolUpdates", JSON.stringify({
                method: "pointingAt",
                desktopWindow: desktopWindow,
                tablet: tablet,
                rightHand: rightHand
            }));
        };

        this.exitModule = function() {
            return makeRunningValues(false, [], []);
        };

        this.isReady = function(controllerData) {
            if (isInEditMode()) {
                if (controllerData.triggerValues[this.hand] < TRIGGER_ON_VALUE) {
                    this.triggerClicked = false;
                }
                Messages.sendLocalMessage('Hifi-unhighlight-all', '');
                return makeRunningValues(true, [], []);
            }
            this.triggerClicked = false;
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

            var overlayLaser = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightWebSurfaceLaserInput" : "LeftWebSurfaceLaserInput");
            if (overlayLaser) {
                var overlayLaserReady = overlayLaser.isReady(controllerData);
                var target = controllerData.rayPicks[this.hand].objectID;
                this.sendPointingAtData(controllerData);
                if (overlayLaserReady.active && this.pointingAtTablet(target)) {
                    return this.exitModule();
                }
            }

            var nearOverlay = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightNearParentingGrabOverlay" : "LeftNearParentingGrabOverlay");
            if (nearOverlay) {
                var nearOverlayReady = nearOverlay.isReady(controllerData);

                if (nearOverlayReady.active && HMD.tabletID && nearOverlay.grabbedThingID === HMD.tabletID) {
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
    }

    var leftHandInEditMode = new InEditMode(LEFT_HAND);
    var rightHandInEditMode = new InEditMode(RIGHT_HAND);

    enableDispatcherModule("LeftHandInEditMode", leftHandInEditMode);
    enableDispatcherModule("RightHandInEditMode", rightHandInEditMode);

    function cleanup() {
        disableDispatcherModule("LeftHandInEditMode");
        disableDispatcherModule("RightHandInEditMode");
    }

    Script.scriptEnding.connect(cleanup);
}());
