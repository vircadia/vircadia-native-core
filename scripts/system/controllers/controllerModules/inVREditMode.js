"use strict";

//  inVREditMode.js
//
//  Created by David Rowe on 16 Sep 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, MyAvatar, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule,
   makeDispatcherModuleParameters, makeRunningValues, getEnabledModuleByName, makeLaserParams
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");

(function () {

    function InVREditMode(hand) {
        this.hand = hand;
        this.disableModules = false;
        var NO_HAND_LASER = -1; // Invalid hand parameter so that default laser is not displayed.
        this.parameters = makeDispatcherModuleParameters(
            240, // Not too high otherwise the tablet laser doesn't work.
            this.hand === RIGHT_HAND
                ? ["rightHand", "rightHandEquip", "rightHandTrigger"]
                : ["leftHand", "leftHandEquip", "leftHandTrigger"],
            [],
            100,
            makeLaserParams(NO_HAND_LASER, false)
        );

        this.pointingAtTablet = function (objectID) {
            return (HMD.tabletScreenID && objectID === HMD.tabletScreenID)
                || (HMD.homeButtonID && objectID === HMD.homeButtonID);
        };

        this.isReady = function (controllerData) {
            if (this.disableModules) {
                return makeRunningValues(true, [], []);
            }
            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData) {
            // Default behavior if disabling is not enabled.
            if (!this.disableModules) {
                return makeRunningValues(false, [], []);
            }

            // Tablet stylus.
            var tabletStylusInput = getEnabledModuleByName(this.hand === RIGHT_HAND
                ? "RightTabletStylusInput"
                : "LeftTabletStylusInput");
            if (tabletStylusInput) {
                var tabletReady = tabletStylusInput.isReady(controllerData);
                if (tabletReady.active) {
                    return makeRunningValues(false, [], []);
                }
            }

            // Tablet surface.
            var overlayLaser = getEnabledModuleByName(this.hand === RIGHT_HAND
                ? "RightWebSurfaceLaserInput"
                : "LeftWebSurfaceLaserInput");
            if (overlayLaser) {
                var overlayLaserReady = overlayLaser.isReady(controllerData);
                var target = controllerData.rayPicks[this.hand].objectID;
                if (overlayLaserReady.active && this.pointingAtTablet(target)) {
                    return makeRunningValues(false, [], []);
                }
            }

            // Tablet grabbing.
            var nearOverlay = getEnabledModuleByName(this.hand === RIGHT_HAND
                ? "RightNearParentingGrabOverlay"
                : "LeftNearParentingGrabOverlay");
            if (nearOverlay) {
                var nearOverlayReady = nearOverlay.isReady(controllerData);
                if (nearOverlayReady.active && HMD.tabletID && nearOverlay.grabbedThingID === HMD.tabletID) {
                    return makeRunningValues(false, [], []);
                }
            }

            // Teleport.
            var teleporter = getEnabledModuleByName(this.hand === RIGHT_HAND
                ? "RightTeleporter"
                : "LeftTeleporter");
            if (teleporter) {
                var teleporterReady = teleporter.isReady(controllerData);
                if (teleporterReady.active) {
                    return makeRunningValues(false, [], []);
                }
            }

            // Other behaviors are disabled.
            return makeRunningValues(true, [], []);
        };
    }

    var leftHandInVREditMode = new InVREditMode(LEFT_HAND);
    var rightHandInVREditMode = new InVREditMode(RIGHT_HAND);
    enableDispatcherModule("LeftHandInVREditMode", leftHandInVREditMode);
    enableDispatcherModule("RightHandInVREditMode", rightHandInVREditMode);

    var INVREDIT_DISABLER_MESSAGE_CHANNEL = "Hifi-InVREdit-Disabler";
    this.handleMessage = function (channel, message, sender) {
        if (sender === MyAvatar.sessionUUID && channel === INVREDIT_DISABLER_MESSAGE_CHANNEL) {
            if (message === "both") {
                leftHandInVREditMode.disableModules = true;
                rightHandInVREditMode.disableModules = true;
            } else if (message === "none") {
                leftHandInVREditMode.disableModules = false;
                rightHandInVREditMode.disableModules = false;
            }
        }
    };
    Messages.subscribe(INVREDIT_DISABLER_MESSAGE_CHANNEL);
    Messages.messageReceived.connect(this.handleMessage);

    this.cleanup = function () {
        disableDispatcherModule("LeftHandInVREditMode");
        disableDispatcherModule("RightHandInVREditMode");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
