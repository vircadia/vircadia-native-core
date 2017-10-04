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

/* global Script, Controller, LaserPointers, RayPick, RIGHT_HAND, LEFT_HAND, Mat4, MyAvatar, Vec3, Camera, Quat,
   getGrabPointSphereOffset, getEnabledModuleByName, makeRunningValues, Entities, NULL_UUID,
   enableDispatcherModule, disableDispatcherModule, entityIsDistanceGrabbable,
   makeDispatcherModuleParameters, MSECS_PER_SEC, HAPTIC_PULSE_STRENGTH, HAPTIC_PULSE_DURATION,
   PICK_MAX_DISTANCE, COLORS_GRAB_SEARCHING_HALF_SQUEEZE, COLORS_GRAB_SEARCHING_FULL_SQUEEZE, COLORS_GRAB_DISTANCE_HOLD,
   AVATAR_SELF_ID, DEFAULT_SEARCH_SPHERE_DISTANCE, TRIGGER_OFF_VALUE, TRIGGER_ON_VALUE, ZERO_VEC, ensureDynamic,
   getControllerWorldLocation, projectOntoEntityXYPlane, ContextOverlay, HMD, Reticle, Overlays, isPointingAtUI

*/
(function() {
    Script.include("/~/system/libraries/controllers.js");
    var ControllerDispatcherUtils = Script.require("/~/system/libraries/controllerDispatcherUtils.js");
    var halfPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawHUDLayer: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };
    var halfEnd = {
        type: "sphere",
        solid: true,
        color: COLORS_GRAB_SEARCHING_HALF_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawHUDLayer: true, // Even when burried inside of something, show it.
        visible: true
    };
    var fullPath = {
        type: "line3d",
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawHUDLayer: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };
    var fullEnd = {
        type: "sphere",
        solid: true,
        color: COLORS_GRAB_SEARCHING_FULL_SQUEEZE,
        alpha: 0.9,
        ignoreRayIntersection: true,
        drawHUDLayer: true, // Even when burried inside of something, show it.
        visible: true
    };
    var holdPath = {
        type: "line3d",
        color: COLORS_GRAB_DISTANCE_HOLD,
        visible: true,
        alpha: 1,
        solid: true,
        glow: 1.0,
        lineWidth: 5,
        ignoreRayIntersection: true, // always ignore this
        drawHUDLayer: true, // Even when burried inside of something, show it.
        parentID: AVATAR_SELF_ID
    };

    var renderStates = [
        {name: "half", path: halfPath, end: halfEnd},
        {name: "full", path: fullPath, end: fullEnd},
        {name: "hold", path: holdPath}
    ];

    var defaultRenderStates = [
        {name: "half", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: halfPath},
        {name: "full", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: fullPath},
        {name: "hold", distance: DEFAULT_SEARCH_SPHERE_DISTANCE, path: holdPath}
    ];

    var MARGIN = 25;

    function HudOverlayPointer(hand) {
        var _this = this;
        this.hand = hand;
        this.reticleMinX = MARGIN;
        this.reticleMaxX;
        this.reticleMinY = MARGIN;
        this.reticleMaxY;
        this.clicked = false;
        this.triggerClicked = 0;
        this.movedAway = false;
        this.parameters = ControllerDispatcherUtils.makeDispatcherModuleParameters(
            540,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.getOtherHandController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.LeftHand : Controller.Standard.RightHand;
        };

        _this.isClicked = function() {
            return _this.triggerClicked;
        };

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.updateRecommendedArea = function() {
            var dims = Controller.getViewportDimensions();
            this.reticleMaxX = dims.x - MARGIN;
            this.reticleMaxY = dims.y - MARGIN;
        };

        this.updateLaserPointer = function(controllerData) {
            var RADIUS = 0.005;
            var dim = { x: RADIUS, y: RADIUS, z: RADIUS };

            if (this.mode === "full") {
                this.fullEnd.dimensions = dim;
                LaserPointers.editRenderState(this.laserPointer, this.mode, {path: fullPath, end: this.fullEnd});
            } else if (this.mode === "half") {
                this.halfEnd.dimensions = dim;
                LaserPointers.editRenderState(this.laserPointer, this.mode, {path: halfPath, end: this.halfEnd});
            }

            LaserPointers.enableLaserPointer(this.laserPointer);
            LaserPointers.setRenderState(this.laserPointer, this.mode);
        };

        this.processControllerTriggers = function(controllerData) {
            if (controllerData.triggerClicks[this.hand]) {
                this.mode = "full";
            } else if (controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE) {
                this.clicked = false;
                this.mode = "half";
            } else {
                this.mode = "none";
            }
        };

        this.calculateNewReticlePosition = function(intersection) {
            this.updateRecommendedArea();
            var point2d = HMD.overlayFromWorldPoint(intersection);
            point2d.x = Math.max(this.reticleMinX, Math.min(point2d.x, this.reticleMaxX));
            point2d.y = Math.max(this.reticleMinY, Math.min(point2d.y, this.reticleMaxY));
            return point2d;
        };

        this.setReticlePosition = function(point2d) {
            Reticle.setPosition(point2d);
        };

        this.pointingAtTablet = function(controllerData) {
            var rayPick = controllerData.rayPicks[this.hand];
            return (rayPick.objectID === HMD.tabletScreenID || rayPick.objectID === HMD.homeButtonID);
        };

        this.processLaser = function(controllerData) {
            var controllerLocation = controllerData.controllerLocations[this.hand];
            if ((controllerData.triggerValues[this.hand] < ControllerDispatcherUtils.TRIGGER_ON_VALUE || !controllerLocation.valid) ||
                this.pointingAtTablet(controllerData)) {
                this.exitModule();
                return false;
            }
            var hudRayPick = controllerData.hudRayPicks[this.hand];
            var point2d = this.calculateNewReticlePosition(hudRayPick.intersection);
            if (!Window.isPointOnDesktopWindow(point2d) && !this.triggerClicked) {
                this.exitModule();
                return false;
            }
            this.setReticlePosition(point2d);
            Reticle.visible = false;
            this.movedAway = false;
            this.triggerClicked = controllerData.triggerClicks[this.hand];
            this.processControllerTriggers(controllerData);
            this.updateLaserPointer(controllerData);
            return true;
        };

        this.exitModule = function() {
            LaserPointers.disableLaserPointer(this.laserPointer);
        };

        this.isReady = function (controllerData) {
            if (this.processLaser(controllerData)) {
                return ControllerDispatcherUtils.makeRunningValues(true, [], []);
            } else {
                return ControllerDispatcherUtils.makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData, deltaTime) {
            return this.isReady(controllerData);
        };

        this.cleanup = function () {
            LaserPointers.disableLaserPointer(this.laserPointer);
            LaserPointers.removeLaserPointer(this.laserPointer);
        };

        this.halfEnd = halfEnd;
        this.fullEnd = fullEnd;
        this.laserPointer = LaserPointers.createLaserPointer({
            joint: (this.hand === RIGHT_HAND) ? "_CONTROLLER_RIGHTHAND" : "_CONTROLLER_LEFTHAND",
            filter: RayPick.PICK_HUD,
            maxDistance: PICK_MAX_DISTANCE,
            posOffset: getGrabPointSphereOffset(this.handToController(), true),
            renderStates: renderStates,
            enabled: true,
            defaultRenderStates: defaultRenderStates
        });
    }


    var leftHudOverlayPointer = new HudOverlayPointer(LEFT_HAND);
    var rightHudOverlayPointer = new HudOverlayPointer(RIGHT_HAND);

    var clickMapping = Controller.newMapping('HudOverlayPointer-click');
    clickMapping.from(rightHudOverlayPointer.isClicked).to(Controller.Actions.ReticleClick);
    clickMapping.from(leftHudOverlayPointer.isClicked).to(Controller.Actions.ReticleClick);
    clickMapping.enable();

    ControllerDispatcherUtils.enableDispatcherModule("LeftHudOverlayPointer", leftHudOverlayPointer);
    ControllerDispatcherUtils.enableDispatcherModule("RightHudOverlayPointer", rightHudOverlayPointer);

    function cleanup() {
        ControllerDispatcherUtils.disableDispatcherModule("LeftHudOverlayPointer");
        ControllerDispatcherUtils.disableDispatcherModule("RightHudOverlayPointer");
    }
    Script.scriptEnding.connect(cleanup);

})();
