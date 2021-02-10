"use strict";

//  inEditMode.js
//
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Controller, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, makeDispatcherModuleParameters, HMD, getEnabledModuleByName, TRIGGER_ON_VALUE, isInEditMode, Picks,
   makeLaserParams
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");
Script.include("/~/system/libraries/utils.js");

(function () {
    var MARGIN = 25;
    function InEditMode(hand) {
        this.hand = hand;
        this.isEditing = false;
        this.triggerClicked = false;
        this.selectedTarget = null;
        this.reticleMinX = MARGIN;
        this.reticleMaxX = null;
        this.reticleMinY = MARGIN;
        this.reticleMaxY = null;

        this.parameters = makeDispatcherModuleParameters(
            165, // Lower priority than webSurfaceLaserInput and hudOverlayPointer.
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
            return (HMD.tabletScreenID && objectID === HMD.tabletScreenID) ||
                (HMD.homeButtonID && objectID === HMD.homeButtonID);
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

        this.ENTITY_TOOL_UPDATES_CHANNEL = "entityToolUpdates";

        this.sendPickData = function(controllerData) {
            if (controllerData.triggerClicks[this.hand]) {
                var hand = this.hand === RIGHT_HAND ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
                if (!this.triggerClicked) {
                    this.selectedTarget = controllerData.rayPicks[this.hand];
                    if (!this.selectedTarget.intersects) {
                        Messages.sendLocalMessage(this.ENTITY_TOOL_UPDATES_CHANNEL, JSON.stringify({
                            method: "clearSelection",
                            hand: hand
                        }));
                    } else {
                        if (this.selectedTarget.type === Picks.INTERSECTED_ENTITY) {
                            Messages.sendLocalMessage(this.ENTITY_TOOL_UPDATES_CHANNEL, JSON.stringify({
                                method: "selectEntity",
                                entityID: this.selectedTarget.objectID,
                                hand: hand,
                                surfaceNormal: this.selectedTarget.surfaceNormal,
                                intersection: this.selectedTarget.intersection
                            }));
                        } else if (this.selectedTarget.type === Picks.INTERSECTED_OVERLAY) {
                            Messages.sendLocalMessage(this.ENTITY_TOOL_UPDATES_CHANNEL, JSON.stringify({
                                method: "selectOverlay",
                                overlayID: this.selectedTarget.objectID,
                                hand: hand
                            }));
                        }
                    }
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
            Messages.sendLocalMessage(this.ENTITY_TOOL_UPDATES_CHANNEL, JSON.stringify({
                method: "pointingAt",
                desktopWindow: desktopWindow,
                tablet: tablet,
                rightHand: rightHand
            }));
        };

        this.runModule = function() {
            return makeRunningValues(true, [], []);
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
                return this.runModule();
            }
            this.triggerClicked = false;
            return this.exitModule();
        };

        this.run = function(controllerData) {

            // Tablet stylus.
            var tabletStylusInput = getEnabledModuleByName(this.hand === RIGHT_HAND
                ? "RightTabletStylusInput" : "LeftTabletStylusInput");
            if (tabletStylusInput) {
                var tabletReady = tabletStylusInput.isReady(controllerData);
                if (tabletReady.active) {
                    return this.exitModule();
                }
            }

            // Tablet surface.
            var webLaser = getEnabledModuleByName(this.hand === RIGHT_HAND
                ? "RightWebSurfaceLaserInput" : "LeftWebSurfaceLaserInput");
            if (webLaser) {
                var webLaserReady = webLaser.isReady(controllerData);
                var target = controllerData.rayPicks[this.hand].objectID;
                this.sendPointingAtData(controllerData);
                if (webLaserReady.active && this.pointingAtTablet(target)) {
                    return this.exitModule();
                }
            }

            // HUD overlay.
            if (!controllerData.triggerClicks[this.hand]) { // Don't grab if trigger pressed when laser starts intersecting.
                var hudLaser = getEnabledModuleByName(this.hand === RIGHT_HAND
                    ? "RightHudOverlayPointer" : "LeftHudOverlayPointer");
                if (hudLaser) {
                    var hudLaserReady = hudLaser.isReady(controllerData);
                    if (hudLaserReady.active) {
                        return this.exitModule();
                    }
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

            // Teleport.
            var teleport = getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightTeleporter" : "LeftTeleporter");
            if (teleport) {
                var teleportReady = teleport.isReady(controllerData);
                if (teleportReady.active) {
                    return this.exitModule();
                }
            }

            if ((controllerData.triggerClicks[this.hand] === 0 && controllerData.secondaryValues[this.hand] === 0)) {
                var stopRunning = false;
                controllerData.nearbyOverlayIDs[this.hand].forEach(function(overlayID) {
                    var overlayName = Overlays.getProperty(overlayID, "name");
                    if (overlayName === "KeyboardAnchor") {
                        stopRunning = true;
                    }
                });

                if (stopRunning) {
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

    var INEDIT_STATUS_CHANNEL = "Hifi-InEdit-Status";
    var HAND_RAYPICK_BLACKLIST_CHANNEL = "Hifi-Hand-RayPick-Blacklist";
    this.handleMessage = function (channel, data, sender) {
        if (channel === INEDIT_STATUS_CHANNEL && sender === MyAvatar.sessionUUID) {
            var message;

            try {
                message = JSON.parse(data);
            } catch (e) {
                return;
            }

            switch (message.method) {
                case "editing":
                    if (message.hand === LEFT_HAND) {
                        leftHandInEditMode.isEditing = message.editing;
                    } else {
                        rightHandInEditMode.isEditing = message.editing;
                    }
                    Messages.sendLocalMessage(HAND_RAYPICK_BLACKLIST_CHANNEL, JSON.stringify({
                        action: "tablet",
                        hand: message.hand,
                        blacklist: message.editing
                    }));
                    break;
            }
        }
    };
    Messages.subscribe(INEDIT_STATUS_CHANNEL);
    Messages.messageReceived.connect(this.handleMessage);

    function cleanup() {
        disableDispatcherModule("LeftHandInEditMode");
        disableDispatcherModule("RightHandInEditMode");
    }

    Script.scriptEnding.connect(cleanup);
}());
