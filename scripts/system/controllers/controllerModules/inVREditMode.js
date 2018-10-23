"use strict";

//  inVREditMode.js
//
//  Created by David Rowe on 16 Sep 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, HMD, Messages, MyAvatar, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule,
   makeDispatcherModuleParameters, makeRunningValues, getEnabledModuleByName, makeLaserParams
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");

(function () {

    function InVREditMode(hand) {
        this.hand = hand;
        this.disableModules = false;
        this.running = false;
        var NO_HAND_LASER = -1; // Invalid hand parameter so that standard laser is not displayed.
        this.parameters = makeDispatcherModuleParameters(
            166, // Slightly lower priority than inEditMode.
            this.hand === RIGHT_HAND
                ? ["rightHand", "rightHandEquip", "rightHandTrigger"]
                : ["leftHand", "leftHandEquip", "leftHandTrigger"],
            [],
            100,
            makeLaserParams(NO_HAND_LASER, false)
        );

        this.pointingAtTablet = function (objectID) {
            return (HMD.tabletScreenID && objectID === HMD.tabletScreenID) ||
                (HMD.homeButtonID && objectID === HMD.homeButtonID);
        };

        // The Shapes app has a non-standard laser: in particular, the laser end dot displays on its own when the laser is 
        // pointing at the Shapes UI. The laser on/off is controlled by this module but the laser is implemented in the Shapes 
        // app.
        // If, in the future, the Shapes app laser interaction is adopted as a standard UI style then the laser could be 
        // implemented in the controller modules along side the other laser styles.
        var INVREDIT_MODULE_RUNNING = "Hifi-InVREdit-Module-Running";

        this.runModule = function () {
            if (!this.running) {
                Messages.sendLocalMessage(INVREDIT_MODULE_RUNNING, JSON.stringify({
                    hand: this.hand,
                    running: true
                }));
                this.running = true;
            }
            return makeRunningValues(true, [], []);
        };

        this.exitModule = function () {
            if (this.running) {
                Messages.sendLocalMessage(INVREDIT_MODULE_RUNNING, JSON.stringify({
                    hand: this.hand,
                    running: false
                }));
                this.running = false;
            }
            return makeRunningValues(false, [], []);
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
                return this.exitModule();
            }

            // Tablet stylus.
            var tabletStylusInput = getEnabledModuleByName(this.hand === RIGHT_HAND ?
                                                           "RightTabletStylusInput" :
                                                           "LeftTabletStylusInput");
            if (tabletStylusInput) {
                var tabletReady = tabletStylusInput.isReady(controllerData);
                if (tabletReady.active) {
                    return this.exitModule();
                }
            }

            // Tablet surface.
            var overlayLaser = getEnabledModuleByName(this.hand === RIGHT_HAND ?
                                                      "RightWebSurfaceLaserInput" :
                                                      "LeftWebSurfaceLaserInput");
            if (overlayLaser) {
                var overlayLaserReady = overlayLaser.isReady(controllerData);
                var target = controllerData.rayPicks[this.hand].objectID;
                if (overlayLaserReady.active && this.pointingAtTablet(target)) {
                    return this.exitModule();
                }
            }

            // Tablet highlight and grabbing.
            var tabletHighlight = getEnabledModuleByName(this.hand === RIGHT_HAND
                ? "RightNearTabletHighlight" : "LeftNearTabletHighlight");
            if (tabletHighlight) {
                var tabletHighlightReady = tabletHighlight.isReady(controllerData);
                if (tabletHighlightReady.active) {
                    return this.exitModule();
                }
            }

            // HUD overlay.
            if (!controllerData.triggerClicks[this.hand]) {
                var hudLaser = getEnabledModuleByName(this.hand === RIGHT_HAND
                    ? "RightHudOverlayPointer"
                    : "LeftHudOverlayPointer");
                if (hudLaser) {
                    var hudLaserReady = hudLaser.isReady(controllerData);
                    if (hudLaserReady.active) {
                        return this.exitModule();
                    }
                }
            }

            // Teleport.
            var teleporter = getEnabledModuleByName(this.hand === RIGHT_HAND ?
                                                    "RightTeleporter" :
                                                    "LeftTeleporter");
            if (teleporter) {
                var teleporterReady = teleporter.isReady(controllerData);
                if (teleporterReady.active) {
                    return this.exitModule();
                }
            }

            // Other behaviors are disabled.
            return this.runModule();
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
