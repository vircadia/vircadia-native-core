"use strict";

//  webSurfaceLaserInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, Controller, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule,
   makeRunningValues, Messages, Quat, Vec3, makeDispatcherModuleParameters, Overlays, ZERO_VEC, HMD,
   INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, getGrabPointSphereOffset, COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE,
   TRIGGER_OFF_VALUE, getEnabledModuleByName, PICK_MAX_DISTANCE, LaserPointers, RayPick, ContextOverlay, Picks, makeLaserParams
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    function WebSurfaceLaserInput(hand) {
        this.hand = hand;
        this.otherHand = this.hand === RIGHT_HAND ? LEFT_HAND : RIGHT_HAND;
        this.running = false;

        this.parameters = makeDispatcherModuleParameters(
            120,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100,
            makeLaserParams(hand, true));

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

        this.getOtherModule = function() {
            return this.hand === RIGHT_HAND ? leftOverlayLaserInput : rightOverlayLaserInput;
        };

        this.isPointingAtWebEntity = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            var entityProperty = Entities.getEntityProperties(intersection.objectID);
            var entityType = entityProperty.type;
            if ((intersection.type === Picks.INTERSECTED_ENTITY && entityType === "Web")) {
                return true;
            }
            return false;
        };

        this.isPointingAtOverlay = function(controllerData) {
            var intersection = controllerData.rayPicks[this.hand];
            return intersection.type === Picks.INTERSECTED_OVERLAY;
        };

        this.deleteContextOverlay = function() {
            var farGrabModule = getEnabledModuleByName(this.hand === RIGHT_HAND
                ? "RightFarActionGrabEntity" : "LeftFarActionGrabEntity");
            if (farGrabModule) {
                var entityWithContextOverlay = farGrabModule.entityWithContextOverlay;

                if (entityWithContextOverlay) {
                    ContextOverlay.destroyContextOverlay(entityWithContextOverlay);
                    farGrabModule.entityWithContextOverlay = false;
                }
            }
        };

        this.updateAllwaysOn = function() {
            var PREFER_STYLUS_OVER_LASER = "preferStylusOverLaser";
            this.parameters.handLaser.allwaysOn = !Settings.getValue(PREFER_STYLUS_OVER_LASER, false);
        };

        this.getDominantHand = function() {
            return MyAvatar.getDominantHand() === "right" ? 1 : 0;
        };

        this.letOtherHandRunFirst = function (controllerData, pointingAt) {
            // If both hands are ready to run, let the other hand run first if it is the dominant hand so that it gets the
            // highlight.
            var isOtherTriggerPressed = controllerData.triggerValues[this.otherHand] > TRIGGER_OFF_VALUE;
            var isLetOtherHandRunFirst = !this.getOtherModule().running
                && this.getDominantHand() === this.otherHand
                && (this.parameters.handLaser.allwaysOn || isOtherTriggerPressed);
            if (isLetOtherHandRunFirst) {
                var otherHandPointingAt = controllerData.rayPicks[this.otherHand].objectID;
                if (this.isTabletID(otherHandPointingAt)) {
                    otherHandPointingAt = HMD.tabletID;
                }
                isLetOtherHandRunFirst = pointingAt === otherHandPointingAt;
            }
            return isLetOtherHandRunFirst;
        };

        this.hoverItem = null;

        this.isTabletID = function (uuid) {
            return [HMD.tabletID, HMD.tabletScreenID, HMD.homeButtonID, HMD.homeButtonHighlightID].indexOf(uuid) !== -1;
        };

        this.isReady = function(controllerData) {
            if (this.isPointingAtOverlay(controllerData) || this.isPointingAtWebEntity(controllerData)) {
                this.updateAllwaysOn();
                
                var isTriggerPressed = controllerData.triggerValues[this.hand] > TRIGGER_OFF_VALUE;
                if (this.parameters.handLaser.allwaysOn || isTriggerPressed) {
                    var pointingAt = controllerData.rayPicks[this.hand].objectID;
                    if (this.isTabletID(pointingAt)) {
                        pointingAt = HMD.tabletID;
                    }

                    if (!this.letOtherHandRunFirst(controllerData, pointingAt)) {

                        if (pointingAt !== this.getOtherModule().hoverItem) {
                            this.parameters.handLaser.doesHover = true;
                            this.hoverItem = pointingAt;
                        } else {
                            this.parameters.handLaser.doesHover = false;
                            this.hoverItem = null;
                        }

                        return makeRunningValues(true, [], []);
                    }
                }
            }

            this.parameters.handLaser.doesHover = false;
            this.hoverItem = null;

            return makeRunningValues(false, [], []);
        };

        this.run = function(controllerData, deltaTime) {
            var grabModuleNeedsToRun = this.grabModuleWantsNearbyOverlay(controllerData);
            var isTriggerPressed = controllerData.triggerValues[this.hand] > TRIGGER_OFF_VALUE;
            if (!grabModuleNeedsToRun && (isTriggerPressed || this.parameters.handLaser.allwaysOn
                    && (this.isPointingAtOverlay(controllerData) || this.isPointingAtWebEntity(controllerData)))) {
                this.running = true;

                var pointingAt = controllerData.rayPicks[this.hand].objectID;
                if (this.isTabletID(pointingAt)) {
                    pointingAt = HMD.tabletID;
                }

                if (pointingAt !== this.getOtherModule().hoverItem || isTriggerPressed) {
                    this.parameters.handLaser.doesHover = true;
                    this.hoverItem = pointingAt;
                } else {
                    this.parameters.handLaser.doesHover = false;
                    this.hoverItem = null;
                }

                return makeRunningValues(true, [], []);
            }
            this.deleteContextOverlay();
            this.running = false;

            this.parameters.handLaser.doesHover = false;
            this.hoverItem = null;

            return makeRunningValues(false, [], []);
        };
    }

    var leftOverlayLaserInput = new WebSurfaceLaserInput(LEFT_HAND);
    var rightOverlayLaserInput = new WebSurfaceLaserInput(RIGHT_HAND);

    enableDispatcherModule("LeftWebSurfaceLaserInput", leftOverlayLaserInput);
    enableDispatcherModule("RightWebSurfaceLaserInput", rightOverlayLaserInput);

    function cleanup() {
        disableDispatcherModule("LeftWebSurfaceLaserInput");
        disableDispatcherModule("RightWebSurfaceLaserInput");
    }
    Script.scriptEnding.connect(cleanup);
}());
