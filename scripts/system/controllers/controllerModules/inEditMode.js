"use strict"

//  inEditMode.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   NULL_UUID, enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, Quat, Vec3, getControllerWorldLocation, makeDispatcherModuleParameters, Overlays, ZERO_VEC,
   AVATAR_SELF_ID, HMD, INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, Settings, getGrabPointSphereOffset
*/

Script.include("/~/system/controllers/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/libraries/utils.js");

(function () {

    function InEditMode(hand) {
        this.hand = hand;

        this.parameters = makeDispatcherModuleParameters(
            160,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            100);

        this.nearTablet = function(overlays) {
            for (var i = 0; i < overlays.length; i++) {
                if (overlays[i] === HMD.tabletID) {
                    return true;
                }
            }

            return false;
        };

        this.pointingAtTablet = function(objectID) {
            if (objectID === HMD.tabletScreenID || objectID === HMD.tabletButtonID) {
                return true;
            }
            return false;
        };

        this.isReady = function(controllerData) {
            var overlays = controllerData.nearbyOverlayIDs[this.hand];
            var objectID = controllerData.rayPicks[this.hand].objectID;

            if (isInEditMode()) {
                return makeRunningValues(true, [], []);
            }

            return makeRunningValues(false, [], []);
        };

        this.run = function(controllerData) {
            var tabletStylusInput = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightTabletStylusInput" : "LeftTabletStylusInput");
            if (tabletStylusInput) {
                var tabletReady = tabletStylusInput.isReady(controllerData);

                if (tabletReady.active) {
                    return makeRunningValues(false, [], []);
                }
            }

            var overlayLaser =  getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightOverlayLaserInput" : "LeftOverlayLaserInput");
            if (overlayLaser) {
                var overlayLaserReady = overlayLaser.isReady(controllerData);

                if (overlayLaserReady.active && this.pointingAtTablet(overlayLaser.target)) {
                    return makeRunningValues(false, [], []);
                }
            }

            var nearOverlay =  getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightNearParentingGrabOverlay" : "LeftNearParentingGrabOverlay");
            if (nearOverlay) {
                var nearOverlayReady = nearOverlay.isReady(controllerData);

                if (nearOverlayReady.active && nearOverlay.grabbedThingID === HMD.tabletID) {
                    return makeRunningValues(false, [], []);
                }
            }

            return this.isReady(controllerData);
        };
    };


    var leftHandInEditMode = new InEditMode(LEFT_HAND);
    var rightHandInEditMode = new InEditMode(RIGHT_HAND);

    enableDispatcherModule("LeftHandInEditMode", leftHandInEditMode);
    enableDispatcherModule("RightHandInEditMode", rightHandInEditMode);

    this.cleanup = function() {
        disableDispatcherModule("LeftHandInEditMode");
        disableDispatcherModule("RightHandInEditMode");
    };
}());
