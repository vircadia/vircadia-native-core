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

    // triggered when stylus presses a web overlay/entity
    var HAPTIC_STYLUS_STRENGTH = 1.0;
    var HAPTIC_STYLUS_DURATION = 20.0;

    var WEB_DISPLAY_STYLUS_DISTANCE = 0.5;
    var WEB_STYLUS_LENGTH = 0.2;
    var WEB_TOUCH_Y_OFFSET = 0.05; // how far forward (or back with a negative number) to slide stylus in hand


    function stylusTargetHasKeyboardFocus(stylusTarget) {
        if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
            return Entities.keyboardFocusEntity === stylusTarget.entityID;
        } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
            return Overlays.keyboardFocusOverlay === stylusTarget.overlayID;
        }
    }

    function setKeyboardFocusOnStylusTarget(stylusTarget) {
        if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID &&
            Entities.wantsHandControllerPointerEvents(stylusTarget.entityID)) {
            Overlays.keyboardFocusOverlay = NULL_UUID;
            Entities.keyboardFocusEntity = stylusTarget.entityID;
        } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
            Overlays.keyboardFocusOverlay = stylusTarget.overlayID;
            Entities.keyboardFocusEntity = NULL_UUID;
        }
    }

    function sendHoverEnterEventToStylusTarget(hand, stylusTarget) {
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: stylusTarget.position2D,
            pos3D: stylusTarget.position,
            normal: stylusTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
            button: "None"
        };

        if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
            Entities.sendHoverEnterEntity(stylusTarget.entityID, pointerEvent);
        } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
            Overlays.sendHoverEnterOverlay(stylusTarget.overlayID, pointerEvent);
        }
    }

    function sendHoverOverEventToStylusTarget(hand, stylusTarget) {
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: stylusTarget.position2D,
            pos3D: stylusTarget.position,
            normal: stylusTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
            button: "None"
        };

        if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
            Entities.sendMouseMoveOnEntity(stylusTarget.entityID, pointerEvent);
            Entities.sendHoverOverEntity(stylusTarget.entityID, pointerEvent);
        } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseMoveOnOverlay(stylusTarget.overlayID, pointerEvent);
            Overlays.sendHoverOverOverlay(stylusTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchStartEventToStylusTarget(hand, stylusTarget) {
        var pointerEvent = {
            type: "Press",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: stylusTarget.position2D,
            pos3D: stylusTarget.position,
            normal: stylusTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
            button: "Primary",
            isPrimaryHeld: true
        };

        if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
            Entities.sendMousePressOnEntity(stylusTarget.entityID, pointerEvent);
            Entities.sendClickDownOnEntity(stylusTarget.entityID, pointerEvent);
        } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
            Overlays.sendMousePressOnOverlay(stylusTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchEndEventToStylusTarget(hand, stylusTarget) {
        var pointerEvent = {
            type: "Release",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: stylusTarget.position2D,
            pos3D: stylusTarget.position,
            normal: stylusTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
            button: "Primary"
        };

        if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
            Entities.sendMouseReleaseOnEntity(stylusTarget.entityID, pointerEvent);
            Entities.sendClickReleaseOnEntity(stylusTarget.entityID, pointerEvent);
            Entities.sendHoverLeaveEntity(stylusTarget.entityID, pointerEvent);
        } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseReleaseOnOverlay(stylusTarget.overlayID, pointerEvent);
        }
    }

    function sendTouchMoveEventToStylusTarget(hand, stylusTarget) {
        var pointerEvent = {
            type: "Move",
            id: hand + 1, // 0 is reserved for hardware mouse
            pos2D: stylusTarget.position2D,
            pos3D: stylusTarget.position,
            normal: stylusTarget.normal,
            direction: Vec3.subtract(ZERO_VEC, stylusTarget.normal),
            button: "Primary",
            isPrimaryHeld: true
        };

        if (stylusTarget.entityID && stylusTarget.entityID !== NULL_UUID) {
            Entities.sendMouseMoveOnEntity(stylusTarget.entityID, pointerEvent);
            Entities.sendHoldingClickOnEntity(stylusTarget.entityID, pointerEvent);
        } else if (stylusTarget.overlayID && stylusTarget.overlayID !== NULL_UUID) {
            Overlays.sendMouseMoveOnOverlay(stylusTarget.overlayID, pointerEvent);
        }
    }

    // will return undefined if overlayID does not exist.
    function calculateStylusTargetFromOverlay(stylusTip, overlayID) {
        var overlayPosition = Overlays.getProperty(overlayID, "position");
        if (overlayPosition === undefined) {
            return;
        }

        // project stylusTip onto overlay plane.
        var overlayRotation = Overlays.getProperty(overlayID, "rotation");
        if (overlayRotation === undefined) {
            return;
        }
        var normal = Vec3.multiplyQbyV(overlayRotation, {x: 0, y: 0, z: 1});
        var distance = Vec3.dot(Vec3.subtract(stylusTip.position, overlayPosition), normal);
        var position = Vec3.subtract(stylusTip.position, Vec3.multiply(normal, distance));

        // calclulate normalized position
        var invRot = Quat.inverse(overlayRotation);
        var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(position, overlayPosition));
        var dpi = Overlays.getProperty(overlayID, "dpi");

        var dimensions;
        if (dpi) {
            // Calculate physical dimensions for web3d overlay from resolution and dpi; "dimensions" property
            // is used as a scale.
            var resolution = Overlays.getProperty(overlayID, "resolution");
            if (resolution === undefined) {
                return;
            }
            resolution.z = 1; // Circumvent divide-by-zero.
            var scale = Overlays.getProperty(overlayID, "dimensions");
            if (scale === undefined) {
                return;
            }
            scale.z = 0.01; // overlay dimensions are 2D, not 3D.
            dimensions = Vec3.multiplyVbyV(Vec3.multiply(resolution, INCHES_TO_METERS / dpi), scale);
        } else {
            dimensions = Overlays.getProperty(overlayID, "dimensions");
            if (dimensions === undefined) {
                return;
            }
            if (!dimensions.z) {
                dimensions.z = 0.01; // sometimes overlay dimensions are 2D, not 3D.
            }
        }
        var invDimensions = { x: 1 / dimensions.x, y: 1 / dimensions.y, z: 1 / dimensions.z };
        var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), DEFAULT_REGISTRATION_POINT);

        // 2D position on overlay plane in meters, relative to the bounding box upper-left hand corner.
        var position2D = {
            x: normalizedPosition.x * dimensions.x,
            y: (1 - normalizedPosition.y) * dimensions.y // flip y-axis
        };

        return {
            entityID: null,
            overlayID: overlayID,
            distance: distance,
            position: position,
            position2D: position2D,
            normal: normal,
            normalizedPosition: normalizedPosition,
            dimensions: dimensions,
            valid: true
        };
    }

    // will return undefined if entity does not exist.
    function calculateStylusTargetFromEntity(stylusTip, props) {
        if (props.rotation === undefined) {
            // if rotation is missing from props object, then this entity has probably been deleted.
            return;
        }

        // project stylus tip onto entity plane.
        var normal = Vec3.multiplyQbyV(props.rotation, {x: 0, y: 0, z: 1});
        Vec3.multiplyQbyV(props.rotation, {x: 0, y: 1, z: 0});
        var distance = Vec3.dot(Vec3.subtract(stylusTip.position, props.position), normal);
        var position = Vec3.subtract(stylusTip.position, Vec3.multiply(normal, distance));

        // generate normalized coordinates
        var invRot = Quat.inverse(props.rotation);
        var localPos = Vec3.multiplyQbyV(invRot, Vec3.subtract(position, props.position));
        var invDimensions = { x: 1 / props.dimensions.x, y: 1 / props.dimensions.y, z: 1 / props.dimensions.z };
        var normalizedPosition = Vec3.sum(Vec3.multiplyVbyV(localPos, invDimensions), props.registrationPoint);

        // 2D position on entity plane in meters, relative to the bounding box upper-left hand corner.
        var position2D = {
            x: normalizedPosition.x * props.dimensions.x,
            y: (1 - normalizedPosition.y) * props.dimensions.y // flip y-axis
        };

        return {
            entityID: props.id,
            entityProps: props,
            overlayID: null,
            distance: distance,
            position: position,
            position2D: position2D,
            normal: normal,
            normalizedPosition: normalizedPosition,
            dimensions: props.dimensions,
            valid: true
        };
    }

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
            return (this.hand === RIGHT_HAND) ? Controller.Standard.LeftHand : Controller.Standard.RightHand;
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
                sendHoverEnterEventToStylusTarget(this.hand, stylusTarget);
            } else if (stylusTarget.overlayID &&
                       stylusTarget.overlayID !== this.hoverOverlay &&
                       stylusTarget.overlayID !== this.getOtherHandController().hoverOverlay) {
                this.hoverOverlay = stylusTarget.overlayID;
                sendHoverEnterEventToStylusTarget(this.hand, stylusTarget);
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

        this.processStylus = function(controllerData) {
            this.updateStylusTip();

            if (!this.stylusTip.valid || this.overlayLaserActive(controllerData)) {
                this.pointFinger(false);
                this.hideStylus();
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
                    stylusTarget = calculateStylusTargetFromEntity(this.stylusTip, candidateEntities[i]);
                    if (stylusTarget) {
                        stylusTargets.push(stylusTarget);
                    }
                }
            }

            // add the tabletScreen, if it is valid
            if (HMD.tabletScreenID && HMD.tabletScreenID !== NULL_UUID &&
                Overlays.getProperty(HMD.tabletScreenID, "visible")) {
                stylusTarget = calculateStylusTargetFromOverlay(this.stylusTip, HMD.tabletScreenID);
                if (stylusTarget) {
                    stylusTargets.push(stylusTarget);
                }
            }

            // add the tablet home button.
            if (HMD.homeButtonID && HMD.homeButtonID !== NULL_UUID &&
                Overlays.getProperty(HMD.homeButtonID, "visible")) {
                stylusTarget = calculateStylusTargetFromOverlay(this.stylusTip, HMD.homeButtonID);
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
                nearestStylusTarget.distance < SCALED_TABLET_MAX_HOVER_DISTANCE) {

                this.requestTouchFocus(nearestStylusTarget);

                if (!stylusTargetHasKeyboardFocus(nearestStylusTarget)) {
                    setKeyboardFocusOnStylusTarget(nearestStylusTarget);
                }

                if (this.hasTouchFocus(nearestStylusTarget)) {
                    sendHoverOverEventToStylusTarget(this.hand, nearestStylusTarget);
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
            sendTouchStartEventToStylusTarget(this.hand, this.stylusTarget);
            Controller.triggerHapticPulse(HAPTIC_STYLUS_STRENGTH, HAPTIC_STYLUS_DURATION, this.hand);

            this.touchingEnterTimer = 0;
            this.touchingEnterStylusTarget = this.stylusTarget;
            this.deadspotExpired = false;

            var TOUCH_PRESS_TO_MOVE_DEADSPOT = 0.0381;
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
                sendTouchEndEventToStylusTarget(this.hand, this.stylusTarget);
            } else {
                sendTouchEndEventToStylusTarget(this.hand, this.touchingEnterStylusTarget);
            }
        };

        this.stylusTouching = function (controllerData, dt) {

            this.touchingEnterTimer += dt;

            if (this.stylusTarget.entityID) {
                this.stylusTarget = calculateStylusTargetFromEntity(this.stylusTip, this.stylusTarget.entityProps);
            } else if (this.stylusTarget.overlayID) {
                this.stylusTarget = calculateStylusTargetFromOverlay(this.stylusTip, this.stylusTarget.overlayID);
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
                        sendTouchMoveEventToStylusTarget(this.hand, this.stylusTarget);
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
            var overlayLaserModule =
                getEnabledModuleByName(this.hand === RIGHT_HAND ? "RightOverlayLaserInput" : "LeftOverlayLaserInput");
            if (overlayLaserModule) {
                return overlayLaserModule.isReady(controllerData).active;
            }
            return false;
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
