"use strict";

// Created by james b. pollack @imgntn on 7/2/2016
// Copyright 2016 High Fidelity, Inc.
//
//  Creates a beam and target and then teleports you there.  Release when its close to you to cancel.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND,
   enableDispatcherModule, disableDispatcherModule, Messages, makeDispatcherModuleParameters, makeRunningValues, Vec3,
   HMD, Uuid, AvatarList, Picks, Pointers, PickType
*/

Script.include("/~/system/libraries/Xform.js");
Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() { // BEGIN LOCAL_SCOPE

    var TARGET_MODEL_URL = Script.resolvePath("../../assets/models/teleportationSpotBasev5.fbx");
    var SEAT_MODEL_URL = Script.resolvePath("../../assets/models/teleport-seat.fbx");

    var TARGET_MODEL_DIMENSIONS = { x: 0.6552, y: 0.3063, z: 0.6552 };

    var COLORS_TELEPORT_SEAT = {
        red: 255,
        green: 0,
        blue: 170
    };

    var COLORS_TELEPORT_CAN_TELEPORT = {
        red: 97,
        green: 247,
        blue: 255
    };

    var COLORS_TELEPORT_CANCEL = {
        red: 255,
        green: 184,
        blue: 73
    };

    var handInfo = {
        right: {
            controllerInput: Controller.Standard.RightHand
        },
        left: {
            controllerInput: Controller.Standard.LeftHand
        }
    };

    var cancelPath = {
        color: COLORS_TELEPORT_CANCEL,
        alpha: 1,
        width: 0.025,
        drawInFront: true
    };
    var teleportPath = {
        color: COLORS_TELEPORT_CAN_TELEPORT,
        alpha: 1,
        width: 0.025,
        drawInFront: true
    };
    var seatPath = {
        color: COLORS_TELEPORT_SEAT,
        alpha: 1,
        width: 0.025,
        drawInFront: true
    };
    var teleportEnd = {
        type: "model",
        url: TARGET_MODEL_URL,
        dimensions: TARGET_MODEL_DIMENSIONS,
        ignorePickIntersection: true
    };
    var seatEnd = {
        type: "model",
        url: SEAT_MODEL_URL,
        dimensions: TARGET_MODEL_DIMENSIONS,
        ignorePickIntersection: true
    };


    var teleportRenderStates = [{name: "cancel", path: cancelPath},
        {name: "teleport", path: teleportPath, end: teleportEnd},
        {name: "seat", path: seatPath, end: seatEnd}];

    var DEFAULT_DISTANCE = 8.0;
    var teleportDefaultRenderStates = [{name: "cancel", distance: DEFAULT_DISTANCE, path: cancelPath}];

    var ignoredEntities = [];


    var TELEPORTER_STATES = {
        IDLE: 'idle',
        TARGETTING: 'targetting',
        TARGETTING_INVALID: 'targetting_invalid'
    };

    var TARGET = {
        NONE: 'none', // Not currently targetting anything
        INVISIBLE: 'invisible', // The current target is an invvsible surface
        INVALID: 'invalid', // The current target is invalid (wall, ceiling, etc.)
        SURFACE: 'surface', // The current target is a valid surface
        SEAT: 'seat' // The current target is a seat
    };

    var speed = 7.4422;
    var accelerationAxis = {x: 0.0, y: -5.0, z: 0.0};

    function Teleporter(hand) {
        var _this = this;
        this.hand = hand;
        this.buttonValue = 0;
        this.disabled = false; // used by the 'Hifi-Teleport-Disabler' message handler
        this.active = false;
        this.state = TELEPORTER_STATES.IDLE;
        this.currentTarget = TARGET.INVALID;
        this.currentResult = null;

        this.getOtherModule = function() {
            var otherModule = this.hand === RIGHT_HAND ? leftTeleporter : rightTeleporter;
            return otherModule;
        };


        this.teleporterSelectionName = "teleporterSelection" + hand.toString();
        this.TELEPORTER_SELECTION_STYLE = {
            outlineUnoccludedColor: { red: 0, green: 0, blue: 0 },
            outlineUnoccludedAlpha: 0,
            outlineOccludedColor: { red: 0, green: 0, blue: 0 },
            outlineOccludedAlpha: 0,
            fillUnoccludedColor: { red: 0, green: 0, blue: 0 },
            fillUnoccludedAlpha: 0,
            fillOccludedColor: { red: 0, green: 0, blue: 255 },
            fillOccludedAlpha: 0.84,
            outlineWidth: 0,
            isOutlineSmooth: false
        };


        this.teleportParabolaHandVisible = Pointers.createPointer(PickType.Parabola, {
            joint: (_this.hand === RIGHT_HAND) ? "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" : "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
            dirOffset: { x: 0, y: 1, z: 0.1 },
            posOffset: { x: (_this.hand === RIGHT_HAND) ? 0.03 : -0.03, y: 0.2, z: 0.02 },
            filter: Picks.PICK_ENTITIES,
            faceAvatar: true,
            scaleWithAvatar: true,
            centerEndY: false,
            speed: speed,
            accelerationAxis: accelerationAxis,
            rotateAccelerationWithAvatar: true,
            renderStates: teleportRenderStates,
            defaultRenderStates: teleportDefaultRenderStates,
            maxDistance: 8.0
        });
        this.teleportParabolaHandInvisible = Pointers.createPointer(PickType.Parabola, {
            joint: (_this.hand === RIGHT_HAND) ? "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" : "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
            dirOffset: { x: 0, y: 1, z: 0.1 },
            posOffset: { x: (_this.hand === RIGHT_HAND) ? 0.03 : -0.03, y: 0.2, z: 0.02 },
            filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
            faceAvatar: true,
            scaleWithAvatar: true,
            centerEndY: false,
            speed: speed,
            accelerationAxis: accelerationAxis,
            rotateAccelerationWithAvatar: true,
            renderStates: teleportRenderStates,
            maxDistance: 8.0
        });
        this.teleportParabolaHeadVisible = Pointers.createPointer(PickType.Parabola, {
            joint: "Avatar",
            filter: Picks.PICK_ENTITIES,
            faceAvatar: true,
            scaleWithAvatar: true,
            centerEndY: false,
            speed: speed,
            accelerationAxis: accelerationAxis,
            rotateAccelerationWithAvatar: true,
            renderStates: teleportRenderStates,
            defaultRenderStates: teleportDefaultRenderStates,
            maxDistance: 8.0
        });
        this.teleportParabolaHeadInvisible = Pointers.createPointer(PickType.Parabola, {
            joint: "Avatar",
            filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
            faceAvatar: true,
            scaleWithAvatar: true,
            centerEndY: false,
            speed: speed,
            accelerationAxis: accelerationAxis,
            rotateAccelerationWithAvatar: true,
            renderStates: teleportRenderStates,
            maxDistance: 8.0
        });

        this.addToSelectedItemsList = function (overlays) {
            for (var i = 0, length = overlays.length; i < length; i++) {
                Selection.addToSelectedItemsList(this.teleporterSelectionName, "overlay", overlays[i]);
            }
        };

        this.addToSelectedItemsList(Pointers.getOverlayIDs(this.teleportParabolaHandVisible));
        this.addToSelectedItemsList(Pointers.getOverlayIDs(this.teleportParabolaHandInvisible));
        this.addToSelectedItemsList(Pointers.getOverlayIDs(this.teleportParabolaHeadVisible));
        this.addToSelectedItemsList(Pointers.getOverlayIDs(this.teleportParabolaHeadInvisible));


        this.PLAY_AREA_OVERLAY_MODEL = Script.resolvePath("../../assets/models/trackingSpacev2.fbx");
        this.PLAY_AREA_OVERLAY_MODEL_DIMENSIONS = { x: 2, y: 0.2, z: 2 };
        this.PLAY_AREA_FLOAT_ABOVE_FLOOR = 0.3;
        this.PLAY_AREA_OVERLAY_OFFSET = // Offset from floor.
            { x: 0, y: this.PLAY_AREA_OVERLAY_MODEL_DIMENSIONS.y / 2 + this.PLAY_AREA_FLOAT_ABOVE_FLOOR, z: 0 };
        this.PLAY_AREA_SENSOR_OVERLAY_MODEL = Script.resolvePath("../../assets/models/oculusSensorv2.fbx");
        this.PLAY_AREA_SENSOR_OVERLAY_DIMENSIONS = { x: 0.1198, y: 0.2981, z: 0.1198 };
        this.PLAY_AREA_SENSOR_OVERLAY_ROTATION = Quat.fromVec3Degrees({ x: 0, y: -90, z: 0 });
        this.playAreaSensorPositions = [];
        this.playAreaSensorPositionOverlays = [];
        this.playArea = { x: 0, y: 0 };
        this.playAreaCenterOffset = this.PLAY_AREA_OVERLAY_OFFSET;
        this.isPlayAreaVisible = false;
        this.isPlayAreaAvailable = false;
        this.targetOverlayID = null;

        this.PLAY_AREA_FADE_DELAY = 500;
        this.PLAY_AREA_FADE_DURATION = 200;
        this.PLAY_AREA_FADE_INTERVAL = 25;
        this.PLAY_AREA_BOX_ALPHA = 1.0;
        this.PLAY_AREA_SENSOR_ALPHA = 0.8;
        this.PLAY_AREA_FADE_DELTA = this.PLAY_AREA_FADE_INTERVAL / this.PLAY_AREA_FADE_DURATION;

        this.TELEPORT_SCALE_DURATION = 500;
        this.TELEPORT_SCALE_TIMEOUT = 20;
        this.isTeleportVisible = false;
        this.teleportScaleTimer = null;
        this.teleportScaleStart = 0;
        this.teleportScaleFactor = 0;
        this.teleportScaleMode = "head";

        this.playAreaOverlay = Overlays.addOverlay("model", {
            url: this.PLAY_AREA_OVERLAY_MODEL,
            drawInFront: false,
            visible: false
        });
        Selection.addToSelectedItemsList(this.teleporterSelectionName, "overlay", this.playAreaOverlay);

        this.teleportedPosition = Vec3.ZERO;
        this.TELEPORTED_TARGET_ALPHA = 1.0;
        this.TELEPORTED_TARGET_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 });
        this.teleportedTargetOverlay = Overlays.addOverlay("model", {
            url: TARGET_MODEL_URL,
            alpha: this.TELEPORTED_TARGET_ALPHA,
            visible: false
        });
        Selection.addToSelectedItemsList(this.teleportedTargetOverlay, "overlay", this.playAreaOverlay);

        this.addPlayAreaSensorPositionOverlay = function () {
            var overlay = Overlays.addOverlay("model", {
                url: this.PLAY_AREA_SENSOR_OVERLAY_MODEL,
                dimensions: this.PLAY_AREA_SENSOR_OVERLAY_DIMENSIONS,
                parentID: this.playAreaOverlay,
                localRotation: this.PLAY_AREA_SENSOR_OVERLAY_ROTATION,
                solid: true,
                drawInFront: false,
                visible: false
            });
            this.playAreaSensorPositionOverlays.push(overlay);
            Selection.addToSelectedItemsList(this.teleporterSelectionName, "overlay", overlay);
        };

        this.deletePlayAreaSensorPositionOverlay = function (index) {
            var overlay = this.playAreaSensorPositionOverlays[index];
            Selection.removeFromSelectedItemsList(this.teleporterSelectionName, "overlay", overlay);
            Overlays.deleteOverlay(overlay);
            this.playAreaSensorPositionOverlays.splice(index, 1);
        };

        this.setPlayAreaDimensions = function () {
            var avatarScale = MyAvatar.scale;

            var playAreaOverlayProperties = {
                dimensions:
                    Vec3.multiply(this.teleportScaleFactor * avatarScale, {
                        x: this.playArea.width,
                        y: this.PLAY_AREA_OVERLAY_MODEL_DIMENSIONS.y,
                        z: this.playArea.height
                    })
            };

            if (this.teleportScaleFactor < 1) {
                // Adjust position of playAreOverlay so that its base is at correct height.
                // Always parenting to teleport target is good enough for this.
                var sensorToWorldMatrix = MyAvatar.sensorToWorldMatrix;
                var sensorToWorldRotation = Mat4.extractRotation(MyAvatar.sensorToWorldMatrix);
                var worldToSensorMatrix = Mat4.inverse(sensorToWorldMatrix);
                var avatarSensorPosition = Mat4.transformPoint(worldToSensorMatrix, MyAvatar.position);
                avatarSensorPosition.y = 0;

                var targetRotation = Overlays.getProperty(this.targetOverlayID, "rotation");
                var relativePlayAreaCenterOffset =
                    Vec3.sum(this.playAreaCenterOffset, { x: 0, y: -TARGET_MODEL_DIMENSIONS.y / 2, z: 0 });
                var localPosition = Vec3.multiplyQbyV(Quat.inverse(targetRotation),
                    Vec3.multiplyQbyV(sensorToWorldRotation,
                        Vec3.multiply(MyAvatar.scale, Vec3.subtract(relativePlayAreaCenterOffset, avatarSensorPosition))));
                localPosition.y = this.teleportScaleFactor * localPosition.y;

                playAreaOverlayProperties.parentID = this.targetOverlayID;
                playAreaOverlayProperties.localPosition = localPosition;
            }

            Overlays.editOverlay(this.playAreaOverlay, playAreaOverlayProperties);

            for (var i = 0; i < this.playAreaSensorPositionOverlays.length; i++) {
                localPosition = this.playAreaSensorPositions[i];
                localPosition = Vec3.multiply(avatarScale, localPosition);
                // Position relative to the play area.
                localPosition.y = avatarScale * (this.PLAY_AREA_SENSOR_OVERLAY_DIMENSIONS.y / 2
                    - this.PLAY_AREA_OVERLAY_MODEL_DIMENSIONS.y / 2);
                Overlays.editOverlay(this.playAreaSensorPositionOverlays[i], {
                    dimensions: Vec3.multiply(this.teleportScaleFactor * avatarScale, this.PLAY_AREA_SENSOR_OVERLAY_DIMENSIONS),
                    parentID: this.playAreaOverlay,
                    localPosition: localPosition
                });
            }
        };

        this.updatePlayAreaScale = function () {
            if (this.isPlayAreaAvailable) {
                this.setPlayAreaDimensions();
            }
        };

        this.playAreaFadeTimer = null;
        this.PlayAreaFadeFactor = 1.0;

        this.fadePlayArea = function () {
            var i, length;
            _this.PlayAreaFadeFactor = _this.PlayAreaFadeFactor - _this.PLAY_AREA_FADE_DELTA;
            if (_this.PlayAreaFadeFactor > 0) {
                // Fade.
                Overlays.editOverlay(_this.teleportedTargetOverlay, {
                    alpha: _this.PlayAreaFadeFactor * _this.TELEPORTED_TARGET_ALPHA
                });
                Overlays.editOverlay(_this.playAreaOverlay, { alpha: _this.PlayAreaFadeFactor * _this.PLAY_AREA_BOX_ALPHA });
                var sensorAlpha = _this.PlayAreaFadeFactor * _this.PLAY_AREA_SENSOR_ALPHA;
                for (i = 0, length = _this.playAreaSensorPositionOverlays.length; i < length; i++) {
                    Overlays.editOverlay(_this.playAreaSensorPositionOverlays[i], { alpha: sensorAlpha });
                }
                _this.playAreaFadeTimer = Script.setTimeout(_this.fadePlayArea, _this.PLAY_AREA_FADE_INTERVAL);
            } else {
                // Make invisible.
                Overlays.editOverlay(_this.teleportedTargetOverlay, { visible: false });
                Overlays.editOverlay(_this.playAreaOverlay, { visible: false });
                for (i = 0, length = _this.playAreaSensorPositionOverlays.length; i < length; i++) {
                    Overlays.editOverlay(_this.playAreaSensorPositionOverlays[i], { visible: false });
                }
                _this.playAreaFadeTimer = null;
            }
        };

        this.setPlayAreaVisible = function (visible, targetOverlayID, noFade) {
            if (!this.isPlayAreaAvailable || this.isPlayAreaVisible === visible) {
                return;
            }

            this.isPlayAreaVisible = visible;
            this.targetOverlayID = targetOverlayID;

            if (this.playAreaFadeTimer !== null) {
                Script.clearTimeout(this.playAreaFadeTimer);
                this.playAreaFadeTimer = null;
            }
            if (visible || noFade) {
                // Immediately make visible or invisible.
                this.isPlayAreaVisible = visible;
                Overlays.editOverlay(this.playAreaOverlay, {
                    dimensions: Vec3.ZERO,
                    alpha: this.PLAY_AREA_BOX_ALPHA,
                    visible: visible
                });
                for (var i = 0; i < this.playAreaSensorPositionOverlays.length; i++) {
                    Overlays.editOverlay(this.playAreaSensorPositionOverlays[i], {
                        dimensions: Vec3.ZERO,
                        alpha: this.PLAY_AREA_SENSOR_ALPHA,
                        visible: visible
                    });
                }
                Overlays.editOverlay(this.teleportedTargetOverlay, { visible: false });
            } else {
                // Copy of target at teleported position for fading.
                Overlays.editOverlay(this.teleportedTargetOverlay, {
                    position: Vec3.sum(this.teleportedPosition, {
                        x: 0,
                        y: -getAvatarFootOffset() + MyAvatar.scale * TARGET_MODEL_DIMENSIONS.y / 2,
                        z: 0
                    }),
                    rotation: Quat.multiply(this.TELEPORTED_TARGET_ROTATION, MyAvatar.orientation),
                    dimensions: Vec3.multiply(MyAvatar.scale, TARGET_MODEL_DIMENSIONS),
                    alpha: this.TELEPORTED_TARGET_ALPHA,
                    visible: true
                });

                // Fade out over time.
                this.PlayAreaFadeFactor = 1.0;
                _this.playAreaFadeTimer = Script.setTimeout(this.fadePlayArea, this.PLAY_AREA_FADE_DELAY);
            }
        };

        this.setPlayAreaAvailable = function () {
            this.playArea = HMD.playArea;
            this.isPlayAreaAvailable = HMD.active && this.playArea.width !== 0 && this.playArea.height !== 0;
            if (this.isPlayAreaAvailable) {
                this.playAreaCenterOffset = Vec3.sum({ x: this.playArea.x, y: 0, z: this.playArea.y },
                    this.PLAY_AREA_OVERLAY_OFFSET);

                this.playAreaSensorPositions = HMD.sensorPositions;
                for (var i = 0; i < this.playAreaSensorPositions.length; i++) {
                    if (i > this.playAreaSensorPositionOverlays.length - 1) {
                        this.addPlayAreaSensorPositionOverlay();
                    }
                }

                for (i = this.playAreaSensorPositionOverlays.length; i > this.playAreaSensorPositions.length; i--) {
                    this.deletePlayAreaSensorPositionOverlay(i);
                }

                this.setPlayAreaDimensions();
            } else {
                Overlays.editOverlay(this.playAreaOverlay, { visible: false });
                for (i = 0; i < this.playAreaSensorPositionOverlays.length; i++) {
                    Overlays.editOverlay(this.playAreaSensorPositionOverlays[i], { visible: false });
                }
            }
        };

        this.setPlayAreaAvailable();

        this.updatePlayArea = function (position) {
            var sensorToWorldMatrix = MyAvatar.sensorToWorldMatrix;
            var sensorToWorldRotation = Mat4.extractRotation(MyAvatar.sensorToWorldMatrix);
            var worldToSensorMatrix = Mat4.inverse(sensorToWorldMatrix);
            var avatarSensorPosition = Mat4.transformPoint(worldToSensorMatrix, MyAvatar.position);
            avatarSensorPosition.y = 0;

            var targetXZPosition = { x: position.x, y: 0, z: position.z };
            var avatarXZPosition = MyAvatar.position;
            avatarXZPosition.y = 0;
            var MIN_PARENTING_DISTANCE = 0.2; // Parenting under this distance results in the play area's rotation jittering.
            if (Vec3.distance(targetXZPosition, avatarXZPosition) < MIN_PARENTING_DISTANCE) {
                // Set play area position and rotation in world coordinates with no parenting.
                Overlays.editOverlay(this.playAreaOverlay, {
                    parentID: Uuid.NULL,
                    position: Vec3.sum(position,
                        Vec3.multiplyQbyV(sensorToWorldRotation,
                            Vec3.multiply(MyAvatar.scale, Vec3.subtract(this.playAreaCenterOffset, avatarSensorPosition)))),
                    rotation: sensorToWorldRotation
                });
            } else {
                // Set play area position and rotation in local coordinates with parenting.
                var targetRotation = Overlays.getProperty(this.targetOverlayID, "rotation");
                var sensorToTargetRotation = Quat.multiply(Quat.inverse(targetRotation), sensorToWorldRotation);
                var relativePlayAreaCenterOffset =
                    Vec3.sum(this.playAreaCenterOffset, { x: 0, y: -TARGET_MODEL_DIMENSIONS.y / 2, z: 0 });

                Overlays.editOverlay(this.playAreaOverlay, {
                    parentID: this.targetOverlayID,
                    localPosition: Vec3.multiplyQbyV(Quat.inverse(targetRotation),
                        Vec3.multiplyQbyV(sensorToWorldRotation,
                            Vec3.multiply(MyAvatar.scale, Vec3.subtract(relativePlayAreaCenterOffset, avatarSensorPosition)))),
                    localRotation: sensorToTargetRotation
                });
            }
        };


        this.scaleInTeleport = function () {
            _this.teleportScaleFactor = Math.min((Date.now() - _this.teleportScaleStart) / _this.TELEPORT_SCALE_DURATION, 1);
            Pointers.editRenderState(
                _this.teleportScaleMode === "head" ? _this.teleportParabolaHeadVisible : _this.teleportParabolaHandVisible,
                "teleport",
                {
                    path: teleportPath, // Teleport beam disappears if not included.
                    end: { dimensions: Vec3.multiply(_this.teleportScaleFactor, TARGET_MODEL_DIMENSIONS) }
                }
            );
            _this.setPlayAreaDimensions();
            if (_this.teleportScaleFactor < 1) {
                _this.teleportScaleTimer = Script.setTimeout(_this.scaleInTeleport, _this.TELEPORT_SCALE_TIMEOUT);
            } else {
                _this.teleportScaleTimer = null;
            }
        };

        this.setTeleportVisible = function (visible, mode) {
            // Scales in teleport target and play area when start displaying them.
            if (visible === this.isTeleportVisible) {
                return;
            }

            if (visible) {
                this.teleportScaleMode = mode;
                Pointers.editRenderState(
                    mode === "head" ? _this.teleportParabolaHeadVisible : _this.teleportParabolaHandVisible,
                    "teleport",
                    {
                        path: teleportPath, // Teleport beam disappears if not included.
                        end: { dimensions: Vec3.ZERO }
                    }
                );
                this.teleportScaleStart = Date.now();
                this.teleportScaleFactor = 0;
                this.scaleInTeleport();
            } else {
                if (this.teleportScaleTimer !== null) {
                    Script.clearTimeout(this.teleportScaleTimer);
                    this.teleportScaleTimer = null;
                }
            }

            this.isTeleportVisible = visible;
        };


        this.cleanup = function () {
            Selection.removeListFromMap(this.teleporterSelectionName);
            Pointers.removePointer(this.teleportParabolaHandVisible);
            Pointers.removePointer(this.teleportParabolaHandInvisible);
            Pointers.removePointer(this.teleportParabolaHeadVisible);
            Pointers.removePointer(this.teleportParabolaHeadInvisible);
            Overlays.deleteOverlay(this.teleportedTargetOverlay);
            Overlays.deleteOverlay(this.playAreaOverlay);
            for (var i = 0; i < this.playAreaSensorPositionOverlays.length; i++) {
                Overlays.deleteOverlay(this.playAreaSensorPositionOverlays[i]);
            }
        };

        this.axisButtonStateX = 0; // Left/right axis button pressed.
        this.axisButtonStateY = 0; // Up/down axis button pressed.
        this.BUTTON_TRANSITION_DELAY = 100; // Allow time for transition from direction buttons to touch-pad.

        this.axisButtonChangeX = function (value) {
            if (value !== 0) {
                _this.axisButtonStateX = value;
            } else {
                // Delay direction button release until after teleport possibly pressed.
                Script.setTimeout(function () {
                    _this.axisButtonStateX = value;
                }, _this.BUTTON_TRANSITION_DELAY);
            }
        };

        this.axisButtonChangeY = function (value) {
            if (value !== 0) {
                _this.axisButtonStateY = value;
            } else {
                // Delay direction button release until after teleport possibly pressed.
                Script.setTimeout(function () {
                    _this.axisButtonStateY = value;
                }, _this.BUTTON_TRANSITION_DELAY);
            }
        };

        this.teleportLocked = function () {
            // Lock teleport if in advanced movement mode and have just transitioned from pressing a direction button.
            return Controller.getValue(Controller.Hardware.Application.AdvancedMovement)
                && (_this.axisButtonStateX !== 0 || _this.axisButtonStateY !== 0);
        };

        this.buttonPress = function (value) {
            if (value === 0 || !_this.teleportLocked()) {
                _this.buttonValue = value;
            }
        };

        this.parameters = makeDispatcherModuleParameters(
            80,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.enterTeleport = function() {
            this.state = TELEPORTER_STATES.TARGETTING;
        };

        this.isReady = function(controllerData, deltaTime) {
            var otherModule = this.getOtherModule();
            if (!this.disabled && this.buttonValue !== 0 && !otherModule.active) {
                this.active = true;
                this.enterTeleport();
                return makeRunningValues(true, [], []);
            }
            return makeRunningValues(false, [], []);
        };

        this.run = function(controllerData, deltaTime) {

            // Get current hand pose information to see if the pose is valid
            var pose = Controller.getPoseValue(handInfo[(_this.hand === RIGHT_HAND) ? 'right' : 'left'].controllerInput);
            var mode = pose.valid ? _this.hand : 'head';
            if (!pose.valid) {
                Pointers.disablePointer(_this.teleportParabolaHandVisible);
                Pointers.disablePointer(_this.teleportParabolaHandInvisible);
                Pointers.enablePointer(_this.teleportParabolaHeadVisible);
                Pointers.enablePointer(_this.teleportParabolaHeadInvisible);
            } else {
                Pointers.enablePointer(_this.teleportParabolaHandVisible);
                Pointers.enablePointer(_this.teleportParabolaHandInvisible);
                Pointers.disablePointer(_this.teleportParabolaHeadVisible);
                Pointers.disablePointer(_this.teleportParabolaHeadInvisible);
            }

            // We do up to 2 picks to find a teleport location.
            // There are 2 types of teleport locations we are interested in:
            //   1. A visible floor. This can be any entity surface that points within some degree of "up"
            //   2. A seat. The seat can be visible or invisible.
            //
            //  * In the first pass we pick against visible and invisible entities so that we can find invisible seats.
            //    We might hit an invisible entity that is not a seat, so we need to do a second pass.
            //  * In the second pass we pick against visible entities only.
            //
            var result;
            if (mode === 'head') {
                result = Pointers.getPrevPickResult(_this.teleportParabolaHeadInvisible);
            } else {
                result = Pointers.getPrevPickResult(_this.teleportParabolaHandInvisible);
            }

            var teleportLocationType = getTeleportTargetType(result);
            if (teleportLocationType === TARGET.INVISIBLE) {
                if (mode === 'head') {
                    result = Pointers.getPrevPickResult(_this.teleportParabolaHeadVisible);
                } else {
                    result = Pointers.getPrevPickResult(_this.teleportParabolaHandVisible);
                }
                teleportLocationType = getTeleportTargetType(result);
            }

            if (teleportLocationType === TARGET.NONE) {
                // Use the cancel default state
                this.setTeleportState(mode, "cancel", "");
            } else if (teleportLocationType === TARGET.INVALID || teleportLocationType === TARGET.INVISIBLE) {
                this.setTeleportState(mode, "", "cancel");
            } else if (teleportLocationType === TARGET.SURFACE) {
                this.setTeleportState(mode, "teleport", "");
                this.updatePlayArea(result.intersection);
            } else if (teleportLocationType === TARGET.SEAT) {
                this.setTeleportState(mode, "", "seat");
            }
            return this.teleport(result, teleportLocationType);
        };

        this.teleport = function(newResult, target) {
            var result = newResult;
            this.teleportedPosition = newResult.intersection;
            if (_this.buttonValue !== 0) {
                return makeRunningValues(true, [], []);
            }

            if (target === TARGET.NONE || target === TARGET.INVALID) {
                // Do nothing
            } else if (target === TARGET.SEAT) {
                Entities.callEntityMethod(result.objectID, 'sit');
            } else if (target === TARGET.SURFACE) {
                var offset = getAvatarFootOffset();
                result.intersection.y += offset;
                MyAvatar.goToLocation(result.intersection, true, HMD.orientation, false);
                HMD.centerUI();
                MyAvatar.centerBody();
            }

            this.disableLasers();
            this.active = false;
            return makeRunningValues(false, [], []);
        };

        this.disableLasers = function() {
            this.setPlayAreaVisible(false);
            Selection.disableListHighlight(this.teleporterSelectionName);
            Pointers.disablePointer(_this.teleportParabolaHandVisible);
            Pointers.disablePointer(_this.teleportParabolaHandInvisible);
            Pointers.disablePointer(_this.teleportParabolaHeadVisible);
            Pointers.disablePointer(_this.teleportParabolaHeadInvisible);
        };

        this.setTeleportState = function (mode, visibleState, invisibleState) {
            var visible = visibleState === "teleport";
            if (visible) {
                Selection.enableListHighlight(this.teleporterSelectionName, this.TELEPORTER_SELECTION_STYLE);
            } else {
                Selection.disableListHighlight(this.teleporterSelectionName);
            }
            var pointerID;
            if (mode === 'head') {
                Pointers.setRenderState(_this.teleportParabolaHeadVisible, visibleState);
                Pointers.setRenderState(_this.teleportParabolaHeadInvisible, invisibleState);
                pointerID = _this.teleportParabolaHeadVisible;
            } else {
                Pointers.setRenderState(_this.teleportParabolaHandVisible, visibleState);
                Pointers.setRenderState(_this.teleportParabolaHandInvisible, invisibleState);
                pointerID = _this.teleportParabolaHandVisible;
            }
            this.setPlayAreaVisible(visible, Pointers.getEndOverlayID(pointerID, "teleport"), true);
            this.setTeleportVisible(visible);
        };

        this.setIgnoreEntities = function(entitiesToIgnore) {
            Pointers.setIgnoreItems(this.teleportParabolaHandVisible, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportParabolaHandInvisible, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportParabolaHeadVisible, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportParabolaHeadInvisible, entitiesToIgnore);
        };
    }

    // related to repositioning the avatar after you teleport
    var FOOT_JOINT_NAMES = ["RightToe_End", "RightToeBase", "RightFoot"];
    var DEFAULT_ROOT_TO_FOOT_OFFSET = 0.5;
    function getAvatarFootOffset() {

        // find a valid foot jointIndex
        var footJointIndex = -1;
        var i, l = FOOT_JOINT_NAMES.length;
        for (i = 0; i < l; i++) {
            footJointIndex = MyAvatar.getJointIndex(FOOT_JOINT_NAMES[i]);
            if (footJointIndex !== -1) {
                break;
            }
        }
        if (footJointIndex !== -1) {
            // default vertical offset from foot to avatar root.
            var footPos = MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame(footJointIndex);
            if (footPos.x === 0 && footPos.y === 0 && footPos.z === 0.0) {
                // if footPos is exactly zero, it's probably wrong because avatar is currently loading, fall back to default.
                return DEFAULT_ROOT_TO_FOOT_OFFSET * MyAvatar.scale;
            } else {
                return -footPos.y;
            }
        } else {
            return DEFAULT_ROOT_TO_FOOT_OFFSET * MyAvatar.scale;
        }
    }

    var mappingName, teleportMapping;
    var isViveMapped = false;

    function parseJSON(json) {
        try {
            return JSON.parse(json);
        } catch (e) {
            return undefined;
        }
    }
    // When determininig whether you can teleport to a location, the normal of the
    // point that is being intersected with is looked at. If this normal is more
    // than MAX_ANGLE_FROM_UP_TO_TELEPORT degrees from your avatar's up, then
    // you can't teleport there.
    var MAX_ANGLE_FROM_UP_TO_TELEPORT = 70;
    function getTeleportTargetType(result) {
        if (result.type === Picks.INTERSECTED_NONE) {
            return TARGET.NONE;
        }

        var props = Entities.getEntityProperties(result.objectID, ['userData', 'visible']);
        var data = parseJSON(props.userData);
        if (data !== undefined && data.seat !== undefined) {
            var avatarUuid = Uuid.fromString(data.seat.user);
            if (Uuid.isNull(avatarUuid) || !AvatarList.getAvatar(avatarUuid).sessionUUID) {
                return TARGET.SEAT;
            } else {
                return TARGET.INVALID;
            }
        }

        if (!props.visible) {
            return TARGET.INVISIBLE;
        }

        var surfaceNormal = result.surfaceNormal;
        var angle = Math.acos(Vec3.dot(surfaceNormal, Quat.getUp(MyAvatar.orientation))) * (180.0 / Math.PI);

        if (angle > MAX_ANGLE_FROM_UP_TO_TELEPORT) {
            return TARGET.INVALID;
        } else {
            return TARGET.SURFACE;
        }
    }

    function registerViveTeleportMapping() {
        // Disable Vive teleport if touch is transitioning across touch-pad after pressing a direction button.
        if (Controller.Hardware.Vive) {
            var mappingName = 'Hifi-Teleporter-Dev-Vive-' + Math.random();
            var viveTeleportMapping = Controller.newMapping(mappingName);
            viveTeleportMapping.from(Controller.Hardware.Vive.LSX).peek().to(leftTeleporter.axisButtonChangeX);
            viveTeleportMapping.from(Controller.Hardware.Vive.LSY).peek().to(leftTeleporter.axisButtonChangeY);
            viveTeleportMapping.from(Controller.Hardware.Vive.RSX).peek().to(rightTeleporter.axisButtonChangeX);
            viveTeleportMapping.from(Controller.Hardware.Vive.RSY).peek().to(rightTeleporter.axisButtonChangeY);
            Controller.enableMapping(mappingName);
            isViveMapped = true;
        }
    }

    function onHardwareChanged() {
        // Controller.Hardware.Vive is not immediately available at Interface start-up.
        if (!isViveMapped && Controller.Hardware.Vive) {
            registerViveTeleportMapping();
        }
    }

    Controller.hardwareChanged.connect(onHardwareChanged);

    function registerMappings() {
        mappingName = 'Hifi-Teleporter-Dev-' + Math.random();
        teleportMapping = Controller.newMapping(mappingName);

        // Vive teleport button lock-out.
        registerViveTeleportMapping();

        // Teleport actions.
        teleportMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(leftTeleporter.buttonPress);
        teleportMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(rightTeleporter.buttonPress);
    }

    var leftTeleporter = new Teleporter(LEFT_HAND);
    var rightTeleporter = new Teleporter(RIGHT_HAND);

    enableDispatcherModule("LeftTeleporter", leftTeleporter);
    enableDispatcherModule("RightTeleporter", rightTeleporter);
    registerMappings();
    Controller.enableMapping(mappingName);

    function cleanup() {
        Controller.hardwareChanged.disconnect(onHardwareChanged);
        teleportMapping.disable();
        leftTeleporter.cleanup();
        rightTeleporter.cleanup();
        disableDispatcherModule("LeftTeleporter");
        disableDispatcherModule("RightTeleporter");
    }
    Script.scriptEnding.connect(cleanup);

    var handleTeleportMessages = function(channel, message, sender) {
        if (sender === MyAvatar.sessionUUID) {
            if (channel === 'Hifi-Teleport-Disabler') {
                if (message === 'both') {
                    leftTeleporter.disabled = true;
                    rightTeleporter.disabled = true;
                }
                if (message === 'left') {
                    leftTeleporter.disabled = true;
                    rightTeleporter.disabled = false;
                }
                if (message === 'right') {
                    leftTeleporter.disabled = false;
                    rightTeleporter.disabled = true;
                }
                if (message === 'none') {
                    leftTeleporter.disabled = false;
                    rightTeleporter.disabled = false;
                }
            } else if (channel === 'Hifi-Teleport-Ignore-Add' &&
                       !Uuid.isNull(message) &&
                       ignoredEntities.indexOf(message) === -1) {
                ignoredEntities.push(message);
                leftTeleporter.setIgnoreEntities(ignoredEntities);
                rightTeleporter.setIgnoreEntities(ignoredEntities);
            } else if (channel === 'Hifi-Teleport-Ignore-Remove' && !Uuid.isNull(message)) {
                var removeIndex = ignoredEntities.indexOf(message);
                if (removeIndex > -1) {
                    ignoredEntities.splice(removeIndex, 1);
                    leftTeleporter.setIgnoreEntities(ignoredEntities);
                    rightTeleporter.setIgnoreEntities(ignoredEntities);
                }
            }
        }
    };

    Messages.subscribe('Hifi-Teleport-Disabler');
    Messages.subscribe('Hifi-Teleport-Ignore-Add');
    Messages.subscribe('Hifi-Teleport-Ignore-Remove');
    Messages.messageReceived.connect(handleTeleportMessages);

    HMD.displayModeChanged.connect(function () {
        leftTeleporter.setPlayAreaAvailable();
        rightTeleporter.setPlayAreaAvailable();
    });

    MyAvatar.scaleChanged.connect(function () {
        leftTeleporter.updatePlayAreaScale();
        rightTeleporter.updatePlayAreaScale();
    });

}()); // END LOCAL_SCOPE
