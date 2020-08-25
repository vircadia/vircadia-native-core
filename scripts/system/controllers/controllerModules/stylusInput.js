"use strict";

//  stylusInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, MyAvatar, Controller, Uuid, RIGHT_HAND, LEFT_HAND, enableDispatcherModule, disableDispatcherModule,
   makeRunningValues, Vec3, makeDispatcherModuleParameters, Overlays, HMD, Settings, getEnabledModuleByName, Pointers,
   Picks, PickType, Keyboard
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    function isNearStylusTarget(stylusTargets, maxNormalDistance) {
        var stylusTargetIDs = [];
        for (var index = 0; index < stylusTargets.length; index++) {
            var stylusTarget = stylusTargets[index];
            if (stylusTarget.distance <= maxNormalDistance && !(HMD.tabletID && stylusTarget.id === HMD.tabletID)) {
                stylusTargetIDs.push(stylusTarget.id);
            }
        }
        return stylusTargetIDs;
    }

    function getOverlayDistance(controllerPosition, overlayID) {
        var position = Overlays.getProperty(overlayID, "position");
        return {
            id: overlayID,
            distance: Vec3.distance(position, controllerPosition)
        };
    }

    function StylusInput(hand) {
        this.hand = hand;

        this.parameters = makeDispatcherModuleParameters(
            100,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.pointer = Pointers.createPointer(PickType.Stylus, {
            hand: this.hand,
            filter: Picks.PICK_OVERLAYS,
            hover: true,
            enabled: true
        });

        this.disable = false;

        this.otherModuleNeedsToRun = function(controllerData) {
            var grabEntityModuleName = this.hand === RIGHT_HAND ? "RightNearGrabEntity" : "LeftNearGrabEntity";
            var grabEntityModule = getEnabledModuleByName(grabEntityModuleName);
            var grabEntityModuleReady = grabEntityModule ? grabEntityModule.isReady(controllerData) : makeRunningValues(false, [], []);

            var farGrabModuleName = this.hand === RIGHT_HAND ? "RightFarGrabEntity" : "LeftFarGrabEntity";
            var farGrabModule = getEnabledModuleByName(farGrabModuleName);
            var farGrabModuleReady = farGrabModule ? farGrabModule.isReady(controllerData) : makeRunningValues(false, [], []);

            var nearTabletHighlightModuleName =
                this.hand === RIGHT_HAND ? "RightNearTabletHighlight" : "LeftNearTabletHighlight";
            var nearTabletHighlightModule = getEnabledModuleByName(nearTabletHighlightModuleName);
            var nearTabletHighlightModuleReady = nearTabletHighlightModule ?
                nearTabletHighlightModule.isReady(controllerData) : makeRunningValues(false, [], []);

            return farGrabModuleReady.active || grabEntityModuleReady.active;
        };

        this.overlayLaserActive = function(controllerData) {
            var rightOverlayLaserModule = getEnabledModuleByName("RightWebSurfaceLaserInput");
            var leftOverlayLaserModule = getEnabledModuleByName("LeftWebSurfaceLaserInput");
            var rightModuleRunning = rightOverlayLaserModule ? rightOverlayLaserModule.isReady(controllerData).active : false;
            var leftModuleRunning = leftOverlayLaserModule ? leftOverlayLaserModule.isReady(controllerData).active : false;
            return leftModuleRunning || rightModuleRunning;
        };

        this.processStylus = function(controllerData) {
            if (this.overlayLaserActive(controllerData) || this.otherModuleNeedsToRun(controllerData)) {
                Pointers.setRenderState(this.pointer, "disabled");
                return false;
            }

            var sensorScaleFactor = MyAvatar.sensorToWorldScale;

            // build list of stylus targets, near the stylusTip
            var stylusTargets = [];
            var candidateOverlays = controllerData.nearbyOverlayIDs;
            var controllerPosition = controllerData.controllerLocations[this.hand].position;
            var i, stylusTarget;

            for (i = 0; i < candidateOverlays.length; i++) {
                if (!(HMD.tabletID && candidateOverlays[i] === HMD.tabletID) &&
                    Overlays.getProperty(candidateOverlays[i], "visible")) {
                    stylusTarget = getOverlayDistance(controllerPosition, candidateOverlays[i]);
                    if (stylusTarget) {
                        stylusTargets.push(stylusTarget);
                    }
                }
            }

            // add the tabletScreen, if it is valid
            if (HMD.tabletScreenID && HMD.tabletScreenID !== Uuid.NULL &&
                Overlays.getProperty(HMD.tabletScreenID, "visible")) {
                stylusTarget = getOverlayDistance(controllerPosition, HMD.tabletScreenID);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }

            // add the tablet home button.
            if (HMD.homeButtonID && HMD.homeButtonID !== Uuid.NULL &&
                Overlays.getProperty(HMD.homeButtonID, "visible")) {
                stylusTarget = getOverlayDistance(controllerPosition, HMD.homeButtonID);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }

            // Add the mini tablet.
            if (HMD.miniTabletScreenID && Overlays.getProperty(HMD.miniTabletScreenID, "visible") &&
                this.hand != HMD.miniTabletHand) {
                stylusTarget = getOverlayDistance(controllerPosition, HMD.miniTabletScreenID);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }

            var WEB_DISPLAY_STYLUS_DISTANCE = (Keyboard.raised && Keyboard.preferMalletsOverLasers) ? 0.2 : 0.5;
            var nearStylusTarget = isNearStylusTarget(stylusTargets, WEB_DISPLAY_STYLUS_DISTANCE * sensorScaleFactor);

            if (nearStylusTarget.length !== 0) {
                if (!this.disable) {
                    Pointers.setRenderState(this.pointer,"events on");
                    Pointers.setIncludeItems(this.pointer, nearStylusTarget);
                } else {
                    Pointers.setRenderState(this.pointer,"events off");
                }
                return true;
            } else {
                Pointers.setRenderState(this.pointer, "disabled");
                Pointers.setIncludeItems(this.pointer, []);
                return false;
            }
        };

        this.isReady = function (controllerData) {
            var PREFER_STYLUS_OVER_LASER = "preferStylusOverLaser";
            var isUsingStylus = Settings.getValue(PREFER_STYLUS_OVER_LASER, false);

            if (isUsingStylus && this.processStylus(controllerData)) {
                Pointers.enablePointer(this.pointer);
                this.hand === RIGHT_HAND ? Keyboard.disableRightMallet() : Keyboard.disableLeftMallet();
                return makeRunningValues(true, [], []);
            } else {
                Pointers.disablePointer(this.pointer);
                if (Keyboard.raised && Keyboard.preferMalletsOverLasers) {
                    this.hand === RIGHT_HAND ? Keyboard.enableRightMallet() : Keyboard.enableLeftMallet();
                }
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData, deltaTime) {
            return this.isReady(controllerData);
        };

        this.cleanup = function () {
            Pointers.removePointer(this.pointer);
        };
    }

    function mouseHoverEnter(overlayID, event) {
        if (event.id === leftTabletStylusInput.pointer && !rightTabletStylusInput.disable && !leftTabletStylusInput.disable) {
            rightTabletStylusInput.disable = true;
        } else if (event.id === rightTabletStylusInput.pointer && !leftTabletStylusInput.disable && !rightTabletStylusInput.disable) {
            leftTabletStylusInput.disable = true;
        }
    }

    function mouseHoverLeave(overlayID, event) {
        if (event.id === leftTabletStylusInput.pointer) {
            rightTabletStylusInput.disable = false;
        } else if (event.id === rightTabletStylusInput.pointer) {
            leftTabletStylusInput.disable = false;
        }
    }

    var HAPTIC_STYLUS_STRENGTH = 1.0;
    var HAPTIC_STYLUS_DURATION = 20.0;
    function mousePress(overlayID, event) {
        if (HMD.active) {
            if (event.id === leftTabletStylusInput.pointer && event.button === "Primary") {
                Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, LEFT_HAND);
            } else if (event.id === rightTabletStylusInput.pointer && event.button === "Primary") {
                Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, RIGHT_HAND);
            }
        }
    }

    var leftTabletStylusInput = new StylusInput(LEFT_HAND);
    var rightTabletStylusInput = new StylusInput(RIGHT_HAND);

    enableDispatcherModule("LeftTabletStylusInput", leftTabletStylusInput);
    enableDispatcherModule("RightTabletStylusInput", rightTabletStylusInput);

    Overlays.hoverEnterOverlay.connect(mouseHoverEnter);
    Overlays.hoverLeaveOverlay.connect(mouseHoverLeave);
    Overlays.mousePressOnOverlay.connect(mousePress);

    this.cleanup = function () {
        leftTabletStylusInput.cleanup();
        rightTabletStylusInput.cleanup();
        disableDispatcherModule("LeftTabletStylusInput");
        disableDispatcherModule("RightTabletStylusInput");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
