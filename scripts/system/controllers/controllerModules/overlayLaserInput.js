"use strict";

//  overlayLaserInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, Controller, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule,
   makeRunningValues, Messages, Quat, Vec3, makeDispatcherModuleParameters, Overlays, ZERO_VEC, HMD,
   INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, getGrabPointSphereOffset, COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
   COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_ON_VALUE,
   TRIGGER_OFF_VALUE, getEnabledModuleByName, PICK_MAX_DISTANCE, LaserPointers, RayPick, ContextOverlay, Picks
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    function OverlayLaserInput(hand) {
        this.hand = hand;

        this.parameters = makeDispatcherModuleParameters(
            120,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100,
            this.hand);

        this.processLaser = function(controllerData) {
            if (this.shouldExit(controllerData)) {
                return false;
            }
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
            var offOverlay = (intersection.type !== Picks.INTERSECTED_OVERLAY);
            var triggerOff = (controllerData.triggerValues[this.hand] < TRIGGER_OFF_VALUE);
            if (triggerOff) {
                this.deleteContextOverlay();
            }
            var grabbingOverlay = this.grabModuleWantsNearbyOverlay(controllerData);
            return offOverlay || grabbingOverlay || triggerOff;
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
            return this.isReady(controllerData);
        };
    }

    var leftOverlayLaserInput = new OverlayLaserInput(LEFT_HAND);
    var rightOverlayLaserInput = new OverlayLaserInput(RIGHT_HAND);

    enableDispatcherModule("LeftOverlayLaserInput", leftOverlayLaserInput);
    enableDispatcherModule("RightOverlayLaserInput", rightOverlayLaserInput);

    function cleanup() {
        disableDispatcherModule("LeftOverlayLaserInput");
        disableDispatcherModule("RightOverlayLaserInput");
    };
    Script.scriptEnding.connect(cleanup);
}());
