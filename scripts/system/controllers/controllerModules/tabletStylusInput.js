"use strict";

//  tabletStylusInput.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   NULL_UUID, enableDispatcherModule, disableDispatcherModule, makeRunningValues,
   Messages, Quat, Vec3, getControllerWorldLocation, makeDispatcherModuleParameters, Overlays, ZERO_VEC,
   AVATAR_SELF_ID, HMD, INCHES_TO_METERS, DEFAULT_REGISTRATION_POINT, Settings, getGrabPointSphereOffset,
   getEnabledModuleByName
*/

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    var TouchEventUtils = Script.require("/~/system/libraries/touchEventUtils.js");
    // triggered when stylus presses a web overlay/entity
    var HAPTIC_STYLUS_STRENGTH = 1.0;
    var HAPTIC_STYLUS_DURATION = 20.0;

    var WEB_DISPLAY_STYLUS_DISTANCE = 0.5;
    var WEB_STYLUS_LENGTH = 0.2;
    var WEB_TOUCH_Y_OFFSET = 0.105; // how far forward (or back with a negative number) to slide stylus in hand

    function isNearStylusTarget(stylusTargets, edgeBorder, minNormalDistance, maxNormalDistance) {
        for (var i = 0; i < stylusTargets.length; i++) {
            var stylusTarget = stylusTargets[i];

            // check to see if the projected stylusTip is within within the 2d border
            var borderMin = {x: -edgeBorder, y: -edgeBorder};
            var borderMax = {x: stylusTarget.dimensions.x + edgeBorder, y: stylusTarget.dimensions.y + edgeBorder};
            if (stylusTarget.distance >= minNormalDistance && stylusTarget.distance <= maxNormalDistance &&
                stylusTarget.position2D.x >= borderMin.x && stylusTarget.position2D.y >= borderMin.y &&
                stylusTarget.position2D.x <= borderMax.x && stylusTarget.position2D.y <= borderMax.y) {
                return true;
            }
        }
        return false;
    }

    function calculateNearestStylusTarget(stylusTargets) {
        var nearestStylusTarget;

        for (var i = 0; i < stylusTargets.length; i++) {
            var stylusTarget = stylusTargets[i];

            if ((!nearestStylusTarget || stylusTarget.distance < nearestStylusTarget.distance) &&
                stylusTarget.normalizedPosition.x >= 0 && stylusTarget.normalizedPosition.y >= 0 &&
                stylusTarget.normalizedPosition.x <= 1 && stylusTarget.normalizedPosition.y <= 1) {
                nearestStylusTarget = stylusTarget;
            }
        }

        return nearestStylusTarget;
    }

    function getFingerWorldLocation(hand) {
        var fingerJointName = (hand === RIGHT_HAND) ? "RightHandIndex4" : "LeftHandIndex4";

        var fingerJointIndex = MyAvatar.getJointIndex(fingerJointName);
        var fingerPosition = MyAvatar.getAbsoluteJointTranslationInObjectFrame(fingerJointIndex);
        var fingerRotation = MyAvatar.getAbsoluteJointRotationInObjectFrame(fingerJointIndex);
        var worldFingerRotation = Quat.multiply(MyAvatar.orientation, fingerRotation);
        var worldFingerPosition = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, fingerPosition));

        return {
            position: worldFingerPosition,
            orientation: worldFingerRotation,
            rotation: worldFingerRotation,
            valid: true
        };
    }

    function distance2D(a, b) {
        var dx = (a.x - b.x);
        var dy = (a.y - b.y);
        return Math.sqrt(dx * dx + dy * dy);
    }

    function TabletStylusInput(hand) {
        this.hand = hand;
        this.previousStylusTouchingTarget = false;
        this.stylusTouchingTarget = false;

        this.useFingerInsteadOfStylus = false;
        this.fingerPointing = false;

        // initialize stylus tip
        var DEFAULT_STYLUS_TIP = {
            position: {x: 0, y: 0, z: 0},
            orientation: {x: 0, y: 0, z: 0, w: 0},
            rotation: {x: 0, y: 0, z: 0, w: 0},
            velocity: {x: 0, y: 0, z: 0},
            valid: false
        };
        this.stylusTip = DEFAULT_STYLUS_TIP;


        this.parameters = makeDispatcherModuleParameters(
            100,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.getOtherHandController = function() {
            return (this.hand === RIGHT_HAND) ? leftTabletStylusInput : rightTabletStylusInput;
        };

        this.handToController = function() {
            return (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        };

        this.updateFingerAsStylusSetting = function () {
            var DEFAULT_USE_FINGER_AS_STYLUS = false;
            var USE_FINGER_AS_STYLUS = Settings.getValue("preferAvatarFingerOverStylus");
            if (USE_FINGER_AS_STYLUS === "") {
                USE_FINGER_AS_STYLUS = DEFAULT_USE_FINGER_AS_STYLUS;
            }
            if (USE_FINGER_AS_STYLUS && MyAvatar.getJointIndex("LeftHandIndex4") !== -1) {
                this.useFingerInsteadOfStylus = true;
            } else {
                this.useFingerInsteadOfStylus = false;
            }
        };

        this.updateStylusTip = function() {
            if (this.useFingerInsteadOfStylus) {
                this.stylusTip = getFingerWorldLocation(this.hand);
            } else {
                this.stylusTip = getControllerWorldLocation(this.handToController(), true);

                // translate tip forward according to constant.
                var TIP_OFFSET = Vec3.multiply(MyAvatar.sensorToWorldScale, {x: 0, y: WEB_STYLUS_LENGTH - WEB_TOUCH_Y_OFFSET, z: 0});
                this.stylusTip.position = Vec3.sum(this.stylusTip.position,
                    Vec3.multiplyQbyV(this.stylusTip.orientation, TIP_OFFSET));
            }

            // compute tip velocity from hand controller motion, it is more accurate than computing it from previous positions.
            var pose = Controller.getPoseValue(this.handToController());
            if (pose.valid) {
                var worldControllerPos = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation));
                var worldControllerLinearVel = Vec3.multiplyQbyV(MyAvatar.orientation, pose.velocity);
                var worldControllerAngularVel = Vec3.multiplyQbyV(MyAvatar.orientation, pose.angularVelocity);
                var tipVelocity = Vec3.sum(worldControllerLinearVel, Vec3.cross(worldControllerAngularVel,
                    Vec3.subtract(this.stylusTip.position, worldControllerPos)));
                this.stylusTip.velocity = tipVelocity;
            } else {
                this.stylusTip.velocity = {x: 0, y: 0, z: 0};
            }
        };

        this.showStylus = function() {
            if (this.stylus) {
                return;
            }

            var X_ROT_NEG_90 = { x: -0.70710678, y: 0, z: 0, w: 0.70710678 };
            var modelOrientation = Quat.multiply(this.stylusTip.orientation, X_ROT_NEG_90);
            var modelPositionOffset = Vec3.multiplyQbyV(modelOrientation, { x: 0, y: 0, z: MyAvatar.sensorToWorldScale * -WEB_STYLUS_LENGTH / 2 });

            var stylusProperties = {
                name: "stylus",
                url: Script.resourcesPath() + "meshes/tablet-stylus-fat.fbx",
                loadPriority: 10.0,
                position: Vec3.sum(this.stylusTip.position, modelPositionOffset),
                rotation: modelOrientation,
                dimensions: Vec3.multiply(MyAvatar.sensorToWorldScale, { x: 0.01, y: 0.01, z: WEB_STYLUS_LENGTH }),
                solid: true,
                visible: true,
                ignoreRayIntersection: true,
                drawInFront: false,
                parentID: AVATAR_SELF_ID,
                parentJointIndex: MyAvatar.getJointIndex(this.hand === RIGHT_HAND ?
                    "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" :
                    "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND")
            };
            this.stylus = Overlays.addOverlay("model", stylusProperties);
        };

        this.hideStylus = function() {
            if (!this.stylus) {
                return;
            }
            Overlays.deleteOverlay(this.stylus);
            this.stylus = null;
        };

        this.stealTouchFocus = function(stylusTarget) {
            // send hover events to target
            // record the entity or overlay we are hovering over.
            if ((stylusTarget.entityID === this.getOtherHandController().hoverEntity) ||
                (stylusTarget.overlayID === this.getOtherHandController().hoverOverlay)) {
                this.getOtherHandController().relinquishTouchFocus();
            }
            this.requestTouchFocus(stylusTarget);
        };

        this.requestTouchFocus = function(stylusTarget) {

            // send hover events to target if we can.
            // record the entity or overlay we are hovering over.
            if (stylusTarget.entityID &&
                stylusTarget.entityID !== this.hoverEntity &&
                stylusTarget.entityID !== this.getOtherHandController().hoverEntity) {
                this.hoverEntity = stylusTarget.entityID;
                TouchEventUtils.sendHoverEnterEventToTouchTarget(this.hand, stylusTarget);
            } else if (stylusTarget.overlayID &&
                       stylusTarget.overlayID !== this.hoverOverlay &&
                       stylusTarget.overlayID !== this.getOtherHandController().hoverOverlay) {
                this.hoverOverlay = stylusTarget.overlayID;
                TouchEventUtils.sendHoverEnterEventToTouchTarget(this.hand, stylusTarget);
            }
        };

        this.hasTouchFocus = function(stylusTarget) {
            return ((stylusTarget.entityID && stylusTarget.entityID === this.hoverEntity) ||
                    (stylusTarget.overlayID && stylusTarget.overlayID === this.hoverOverlay));
        };

        this.relinquishTouchFocus = function() {
            // send hover leave event.
            var pointerEvent = { type: "Move", id: this.hand + 1 };
            if (this.hoverEntity) {
                Entities.sendHoverLeaveEntity(this.hoverEntity, pointerEvent);
                this.hoverEntity = null;
            } else if (this.hoverOverlay) {
                Overlays.sendMouseMoveOnOverlay(this.hoverOverlay, pointerEvent);
                Overlays.sendHoverOverOverlay(this.hoverOverlay, pointerEvent);
                Overlays.sendHoverLeaveOverlay(this.hoverOverlay, pointerEvent);
                this.hoverOverlay = null;
            }
        };

        this.pointFinger = function(value) {
            var HIFI_POINT_INDEX_MESSAGE_CHANNEL = "Hifi-Point-Index";
            if (this.fingerPointing !== value) {
                var message;
                if (this.hand === RIGHT_HAND) {
                    message = { pointRightIndex: value };
                } else {
                    message = { pointLeftIndex: value };
                }
                Messages.sendMessage(HIFI_POINT_INDEX_MESSAGE_CHANNEL, JSON.stringify(message), true);
                this.fingerPointing = value;
            }
        };

        this.otherModuleNeedsToRun = function(controllerData) {
            var grabOverlayModuleName = this.hand === RIGHT_HAND ? "RightNearParentingGrabOverlay" : "LeftNearParentingGrabOverlay";
            var grabOverlayModule = getEnabledModuleByName(grabOverlayModuleName);
            var grabOverlayModuleReady = grabOverlayModule ? grabOverlayModule.isReady(controllerData) : makeRunningValues(false, [], []);
            var farGrabModuleName = this.hand === RIGHT_HAND ? "RightFarActionGrabEntity" : "LeftFarActionGrabEntity";
            var farGrabModule = getEnabledModuleByName(farGrabModuleName);
            var farGrabModuleReady = farGrabModule ? farGrabModule.isReady(controllerData) : makeRunningValues(false, [], []);
            return grabOverlayModuleReady.active || farGrabModuleReady.active;
        };

        this.processStylus = function(controllerData) {
            this.updateStylusTip();

            if (!this.stylusTip.valid || this.overlayLaserActive(controllerData) || this.otherModuleNeedsToRun(controllerData)) {
                this.pointFinger(false);
                this.hideStylus();
                this.stylusTouchingTarget = false;
                this.relinquishTouchFocus();
                return false;
            }

            if (this.useFingerInsteadOfStylus) {
                this.hideStylus();
            }

            // build list of stylus targets, near the stylusTip
            var stylusTargets = [];
            var candidateEntities = controllerData.nearbyEntityProperties;
            var i, props, stylusTarget;
            for (i = 0; i < candidateEntities.length; i++) {
                props = candidateEntities[i];
                if (props && props.type === "Web") {
                    stylusTarget = TouchEventUtils.calculateTouchTargetFromEntity(this.stylusTip, candidateEntities[i]);
                    if (stylusTarget) {
                        stylusTargets.push(stylusTarget);
                    }
                }
            }

            // add the tabletScreen, if it is valid
            if (HMD.tabletScreenID && HMD.tabletScreenID !== NULL_UUID &&
                Overlays.getProperty(HMD.tabletScreenID, "visible")) {
                stylusTarget = TouchEventUtils.calculateTouchTargetFromOverlay(this.stylusTip, HMD.tabletScreenID);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }

            // add the tablet home button.
            if (HMD.homeButtonID && HMD.homeButtonID !== NULL_UUID &&
                Overlays.getProperty(HMD.homeButtonID, "visible")) {
                stylusTarget = TouchEventUtils.calculateTouchTargetFromOverlay(this.stylusTip, HMD.homeButtonID);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }

            var TABLET_MIN_HOVER_DISTANCE = 0.01;
            var TABLET_MAX_HOVER_DISTANCE = 0.1;
            var TABLET_MIN_TOUCH_DISTANCE = -0.05;
            var TABLET_MAX_TOUCH_DISTANCE = TABLET_MIN_HOVER_DISTANCE;
            var EDGE_BORDER = 0.075;

            var hysteresisOffset = 0.0;
            if (this.isNearStylusTarget) {
                hysteresisOffset = 0.05;
            }

            var sensorScaleFactor = MyAvatar.sensorToWorldScale;
            this.isNearStylusTarget = isNearStylusTarget(stylusTargets,
                (EDGE_BORDER + hysteresisOffset) * sensorScaleFactor,
                (TABLET_MIN_TOUCH_DISTANCE - hysteresisOffset) * sensorScaleFactor,
                (WEB_DISPLAY_STYLUS_DISTANCE + hysteresisOffset) * sensorScaleFactor);

            if (this.isNearStylusTarget) {
                if (!this.useFingerInsteadOfStylus) {
                    this.showStylus();
                } else {
                    this.pointFinger(true);
                }
            } else {
                this.hideStylus();
                this.pointFinger(false);
            }

            var nearestStylusTarget = calculateNearestStylusTarget(stylusTargets);

            var SCALED_TABLET_MIN_TOUCH_DISTANCE = TABLET_MIN_TOUCH_DISTANCE * sensorScaleFactor;
            var SCALED_TABLET_MAX_TOUCH_DISTANCE = TABLET_MAX_TOUCH_DISTANCE * sensorScaleFactor;
            var SCALED_TABLET_MAX_HOVER_DISTANCE = TABLET_MAX_HOVER_DISTANCE * sensorScaleFactor;

            if (nearestStylusTarget && nearestStylusTarget.distance > SCALED_TABLET_MIN_TOUCH_DISTANCE &&
                nearestStylusTarget.distance < SCALED_TABLET_MAX_HOVER_DISTANCE && !this.getOtherHandController().stylusTouchingTarget) {

                this.requestTouchFocus(nearestStylusTarget);

                if (!TouchEventUtils.touchTargetHasKeyboardFocus(nearestStylusTarget)) {
                    TouchEventUtils.setKeyboardFocusOnTouchTarget(nearestStylusTarget);
                }

                if (this.hasTouchFocus(nearestStylusTarget) && !this.stylusTouchingTarget) {
                    TouchEventUtils.sendHoverOverEventToTouchTarget(this.hand, nearestStylusTarget);
                }

                // filter out presses when tip is moving away from tablet.
                // ensure that stylus is within bounding box by checking normalizedPosition
                if (nearestStylusTarget.valid && nearestStylusTarget.distance > SCALED_TABLET_MIN_TOUCH_DISTANCE &&
                    nearestStylusTarget.distance < SCALED_TABLET_MAX_TOUCH_DISTANCE &&
                    Vec3.dot(this.stylusTip.velocity, nearestStylusTarget.normal) < 0 &&
                    nearestStylusTarget.normalizedPosition.x >= 0 && nearestStylusTarget.normalizedPosition.x <= 1 &&
                    nearestStylusTarget.normalizedPosition.y >= 0 && nearestStylusTarget.normalizedPosition.y <= 1) {

                    this.stylusTarget = nearestStylusTarget;
                    this.stylusTouchingTarget = true;
                }
            } else {
                this.relinquishTouchFocus();
            }

            this.homeButtonTouched = false;

            if (this.isNearStylusTarget) {
                return true;
            } else {
                this.pointFinger(false);
                this.hideStylus();
                return false;
            }
        };

        this.stylusTouchingEnter = function () {
            this.stealTouchFocus(this.stylusTarget);
            TouchEventUtils.sendTouchStartEventToTouchTarget(this.hand, this.stylusTarget);
            Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, this.hand);

            this.touchingEnterTimer = 0;
            this.touchingEnterStylusTarget = this.stylusTarget;
            this.deadspotExpired = false;

            var TOUCH_PRESS_TO_MOVE_DEADSPOT = 0.0481;
            this.deadspotRadius = TOUCH_PRESS_TO_MOVE_DEADSPOT;
        };

        this.stylusTouchingExit = function () {

            if (this.stylusTarget === undefined) {
                return;
            }

            // special case to handle home button.
            if (this.stylusTarget.overlayID === HMD.homeButtonID) {
                Messages.sendLocalMessage("home", this.stylusTarget.overlayID);
            }

            // send press event
            if (this.deadspotExpired) {
                TouchEventUtils.sendTouchEndEventToTouchTarget(this.hand, this.stylusTarget);
            } else {
                TouchEventUtils.sendTouchEndEventToTouchTarget(this.hand, this.touchingEnterStylusTarget);
            }
        };

        this.stylusTouching = function (controllerData, dt) {

            this.touchingEnterTimer += dt;

            if (this.stylusTarget.entityID) {
                this.stylusTarget = TouchEventUtils.calculateTouchTargetFromEntity(this.stylusTip, this.stylusTarget.entityProps);
            } else if (this.stylusTarget.overlayID) {
                this.stylusTarget = TouchEventUtils.calculateTouchTargetFromOverlay(this.stylusTip, this.stylusTarget.overlayID);
            }

            var TABLET_MIN_TOUCH_DISTANCE = -0.1;
            var TABLET_MAX_TOUCH_DISTANCE = 0.01;

            if (this.stylusTarget) {
                if (this.stylusTarget.distance > TABLET_MIN_TOUCH_DISTANCE &&
                    this.stylusTarget.distance < TABLET_MAX_TOUCH_DISTANCE) {
                    var POINTER_PRESS_TO_MOVE_DELAY = 0.33; // seconds
                    if (this.deadspotExpired || this.touchingEnterTimer > POINTER_PRESS_TO_MOVE_DELAY ||
                        distance2D(this.stylusTarget.position2D,
                            this.touchingEnterStylusTarget.position2D) > this.deadspotRadius) {
                        TouchEventUtils.sendTouchMoveEventToTouchTarget(this.hand, this.stylusTarget);
                        this.deadspotExpired = true;
                    }
                } else {
                    this.stylusTouchingTarget = false;
                }
            } else {
                this.stylusTouchingTarget = false;
            }
        };

        this.overlayLaserActive = function(controllerData) {
            var rightOverlayLaserModule = getEnabledModuleByName("RightOverlayLaserInput");
            var leftOverlayLaserModule = getEnabledModuleByName("LeftOverlayLaserInput");
            var rightModuleRunning = rightOverlayLaserModule ? !rightOverlayLaserModule.shouldExit(controllerData) : false;
            var leftModuleRunning = leftOverlayLaserModule ? !leftOverlayLaserModule.shouldExit(controllerData) : false;
            return leftModuleRunning || rightModuleRunning;
        };

        this.isReady = function (controllerData) {
            if (this.processStylus(controllerData)) {
                return makeRunningValues(true, [], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.run = function (controllerData, deltaTime) {
            this.updateFingerAsStylusSetting();

            if (!this.previousStylusTouchingTarget && this.stylusTouchingTarget) {
                this.stylusTouchingEnter();
            }
            if (this.previousStylusTouchingTarget && !this.stylusTouchingTarget) {
                this.stylusTouchingExit();
            }
            this.previousStylusTouchingTarget = this.stylusTouchingTarget;

            if (this.stylusTouchingTarget) {
                this.stylusTouching(controllerData, deltaTime);
            }
            if (this.processStylus(controllerData)) {
                return makeRunningValues(true, [], []);
            } else {
                return makeRunningValues(false, [], []);
            }
        };

        this.cleanup = function () {
            this.hideStylus();
        };
    }

    var leftTabletStylusInput = new TabletStylusInput(LEFT_HAND);
    var rightTabletStylusInput = new TabletStylusInput(RIGHT_HAND);

    enableDispatcherModule("LeftTabletStylusInput", leftTabletStylusInput);
    enableDispatcherModule("RightTabletStylusInput", rightTabletStylusInput);

    this.cleanup = function () {
        leftTabletStylusInput.cleanup();
        rightTabletStylusInput.cleanup();
        disableDispatcherModule("LeftTabletStylusInput");
        disableDispatcherModule("RightTabletStylusInput");
    };
    Script.scriptEnding.connect(this.cleanup);
}());
