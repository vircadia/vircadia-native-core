"use strict";

//  webSurfaceLaserInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, Controller, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule,
   makeRunningValues, Messages, Quat, Vec3, makeDispatcherModuleParameters, Overlays, ZERO_VEC, HMD,
   INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, getGrabPointSphereOffset, COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE,
   TRIGGER_OFF_VALUE, getEnabledModuleByName, PICK_MAX_DISTANCE, ContextOverlay, Picks, makeLaserParams
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

        this.dominantHandOverride = false;

        this.isReady = function(controllerData) {
            var otherModuleRunning = this.getOtherModule().running;
            otherModuleRunning = otherModuleRunning && this.getDominantHand() !== this.hand; // Auto-swap to dominant hand.
            var isTriggerPressed = controllerData.triggerValues[this.hand] > TRIGGER_OFF_VALUE
                && controllerData.triggerValues[this.otherHand] <= TRIGGER_OFF_VALUE;
            if ((!otherModuleRunning || isTriggerPressed)
                    && (this.isPointingAtOverlay(controllerData) || this.isPointingAtWebEntity(controllerData))) {
                this.updateAllwaysOn();
                if (isTriggerPressed) {
                    this.dominantHandOverride = true; // Override dominant hand.
                    this.getOtherModule().dominantHandOverride = false;
                }
                if (this.parameters.handLaser.allwaysOn || isTriggerPressed) {
                    return makeRunningValues(true, [], []);
                }
            }
            return makeRunningValues(false, [], []);
        };

        this.run = function(controllerData, deltaTime) {
            var otherModuleRunning = this.getOtherModule().running;
            otherModuleRunning = otherModuleRunning && this.getDominantHand() !== this.hand; // Auto-swap to dominant hand.
            otherModuleRunning = otherModuleRunning || this.getOtherModule().dominantHandOverride; // Override dominant hand.
            var grabModuleNeedsToRun = this.grabModuleWantsNearbyOverlay(controllerData);
            if (!otherModuleRunning && !grabModuleNeedsToRun && (controllerData.triggerValues[this.hand] > TRIGGER_OFF_VALUE
                || this.parameters.handLaser.allwaysOn
                    && (this.isPointingAtOverlay(controllerData) || this.isPointingAtWebEntity(controllerData)))) {
                this.running = true;
                return makeRunningValues(true, [], []);
            }
            this.deleteContextOverlay();
            this.running = false;
            this.dominantHandOverride = false;
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
