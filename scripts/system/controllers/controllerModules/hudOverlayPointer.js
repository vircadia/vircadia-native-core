//
//  hudOverlayPointer.js
//
//  scripts/system/controllers/controllerModules/
//
//  Created by Dante Ruiz 2017-9-21
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Script, Controller, RIGHT_HAND, LEFT_HAND, HMD, makeLaserParams */
(function() {
    Script.include("/~/system/libraries/controllers.js");
    var ControllerDispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");
    var MARGIN = 25;
    var HUD_LASER_OFFSET = 2;
    function HudOverlayPointer(hand) {
        this.hand = hand;
        this.running = false;
        this.reticleMinX = MARGIN;
        this.reticleMaxX;
        this.reticleMinY = MARGIN;
        this.reticleMaxY;
        this.parameters = ControllerDispatcherUtils.makeDispatcherModuleParameters(
            160, // Same as webSurfaceLaserInput.
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100,
            makeLaserParams((this.hand + HUD_LASER_OFFSET), false));

        this.getOtherHandController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.LeftHand : Controller.Standard.RightHand;
        };

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.updateRecommendedArea = function() {
            var dims = Controller.getViewportDimensions();
            this.reticleMaxX = dims.x - MARGIN;
            this.reticleMaxY = dims.y - MARGIN;
        };

        this.calculateNewReticlePosition = function(intersection) {
            this.updateRecommendedArea();
            var point2d = HMD.overlayFromWorldPoint(intersection);
            point2d.x = Math.max(this.reticleMinX, Math.min(point2d.x, this.reticleMaxX));
            point2d.y = Math.max(this.reticleMinY, Math.min(point2d.y, this.reticleMaxY));
            return point2d;
        };

        this.pointingAtTablet = function(controllerData) {
            var rayPick = controllerData.rayPicks[this.hand];
            return (HMD.tabletScreenID && HMD.homeButtonID && (rayPick.objectID === HMD.tabletScreenID || rayPick.objectID === HMD.homeButtonID));
        };

        this.getOtherModule = function() {
            return this.hand === RIGHT_HAND ? leftHudOverlayPointer : rightHudOverlayPointer;
        };

        this.processLaser = function(controllerData) {
            var controllerLocation = controllerData.controllerLocations[this.hand];
            if ((controllerData.triggerValues[this.hand] < ControllerDispatcherUtils.TRIGGER_ON_VALUE || !controllerLocation.valid) ||
                this.pointingAtTablet(controllerData)) {
                return false;
            }
            var hudRayPick = controllerData.hudRayPicks[this.hand];
            var point2d = this.calculateNewReticlePosition(hudRayPick.intersection);
            if (!Window.isPointOnDesktopWindow(point2d) && !this.triggerClicked) {
                return false;
            }

            this.triggerClicked = controllerData.triggerClicks[this.hand];
            return true;
        };

        this.isReady = function (controllerData) {
            var otherModuleRunning = this.getOtherModule().running;
            if (!otherModuleRunning && HMD.active) {
                if (this.processLaser(controllerData)) {
                    this.running = true;
                    return ControllerDispatcherUtils.makeRunningValues(true, [], []);
                } else {
                    this.running = false;
                    return ControllerDispatcherUtils.makeRunningValues(false, [], []);
                }
            }
            return ControllerDispatcherUtils.makeRunningValues(false, [], []);
        };

        this.run = function (controllerData, deltaTime) {
            return this.isReady(controllerData);
        };
    }


    var leftHudOverlayPointer = new HudOverlayPointer(LEFT_HAND);
    var rightHudOverlayPointer = new HudOverlayPointer(RIGHT_HAND);

    ControllerDispatcherUtils.enableDispatcherModule("LeftHudOverlayPointer", leftHudOverlayPointer);
    ControllerDispatcherUtils.enableDispatcherModule("RightHudOverlayPointer", rightHudOverlayPointer);

    function cleanup() {
        ControllerDispatcherUtils.disableDispatcherModule("LeftHudOverlayPointer");
        ControllerDispatcherUtils.disableDispatcherModule("RightHudOverlayPointer");
    }
    Script.scriptEnding.connect(cleanup);

})();
