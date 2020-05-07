"use strict";

// Created by james b. pollack @imgntn on 7/2/2016
// Copyright 2016 High Fidelity, Inc.
//
//  Creates a beam and target and then teleports you there.  Release when its close to you to cancel.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Entities, MyAvatar, Controller, Quat, RIGHT_HAND, LEFT_HAND,
   enableDispatcherModule, disableDispatcherModule, Messages, makeDispatcherModuleParameters, makeRunningValues, Vec3,
   HMD, Uuid, AvatarList, Picks, Pointers, PickType
*/

Script.include("/~/system/libraries/Xform.js");
Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() { // BEGIN LOCAL_SCOPE

    var TARGET_MODEL_URL = Script.resolvePath("../../assets/models/teleportationSpotBasev8.fbx");
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
        alpha: 0.3,
        width: 0.025,
        drawInFront: true
    };
    
    var teleportPath = {
        color: COLORS_TELEPORT_CAN_TELEPORT,
        alpha: 0.7,
        width: 0.025,
        drawInFront: true
    };
    
    var seatPath = {
        color: COLORS_TELEPORT_SEAT,
        alpha: 0.7,
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
    
    var collisionEnd = {
        type: "shape",
        shape: "box",
        dimensions: { x: 1.0, y: 0.001, z: 1.0 },
        alpha: 0.0,
        ignorePickIntersection: true
    };
    
    var teleportRenderStates = [{name: "cancel", path: cancelPath},
        {name: "teleport", path: teleportPath, end: teleportEnd},
        {name: "seat", path: seatPath, end: seatEnd},
        {name: "collision", end: collisionEnd}];

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
        INVALID: 'invalid', // The current target is invalid (wall, ceiling, etc.)
        COLLIDES: 'collides', // Insufficient space to accommodate the avatar capsule
        DISCREPANCY: 'discrepancy', // We are not 100% sure the avatar will fit so we trigger safe landing
        SURFACE: 'surface', // The current target is a valid surface
        SEAT: 'seat' // The current target is a seat
    };

    var speed = 9.3;
    var accelerationAxis = {x: 0.0, y: -5.0, z: 0.0};

    function Teleporter(hand) {
        var _this = this;
        this.init = false;
        this.hand = hand;
        this.buttonValue = 0;
        this.buttonTeleporting = false; // True if buttonValue is controlling teleport.
        this.standardAxisLY = 0.0;
        this.standardAxisRY = 0.0;
        this.disabled = false; // used by the 'Hifi-Teleport-Disabler' message handler
        this.active = false;
        this.state = TELEPORTER_STATES.IDLE;
        this.currentTarget = TARGET.INVALID;
        this.currentResult = null;
        this.capsuleThreshold = 0.05;
        this.pickHeightOffset = 0.05;

        this.getOtherModule = function() {
            var otherModule = this.hand === RIGHT_HAND ? leftTeleporter : rightTeleporter;
            return otherModule;
        };

        this.teleportHeadCollisionPick;
        this.teleportHandCollisionPick;
        this.teleportParabolaHandVisuals;
        this.teleportParabolaHandCollisions;
        this.teleportParabolaHeadVisuals;
        this.teleportParabolaHeadCollisions;


        this.PLAY_AREA_OVERLAY_MODEL = Script.resolvePath("../../assets/models/trackingSpacev18.fbx");
        this.PLAY_AREA_OVERLAY_MODEL_DIMENSIONS = { x: 1.969, y: 0.001, z: 1.969 };
        this.PLAY_AREA_FLOAT_ABOVE_FLOOR = 0.005;
        this.PLAY_AREA_OVERLAY_OFFSET = // Offset from floor.
            { x: 0, y: this.PLAY_AREA_OVERLAY_MODEL_DIMENSIONS.y / 2 + this.PLAY_AREA_FLOAT_ABOVE_FLOOR, z: 0 };
        this.PLAY_AREA_SENSOR_OVERLAY_MODEL = Script.resolvePath("../../assets/models/oculusSensorv11.fbx");
        this.PLAY_AREA_SENSOR_OVERLAY_DIMENSIONS = { x: 0.1198, y: 0.2981, z: 0.1198 };
        this.PLAY_AREA_SENSOR_OVERLAY_ROTATION = Quat.fromVec3Degrees({ x: 0, y: -90, z: 0 });
        this.PLAY_AREA_BOX_ALPHA = 1.0;
        this.PLAY_AREA_SENSOR_ALPHA = 0.8;
        this.playAreaSensorPositions = [];
        this.playArea = { x: 0, y: 0 };
        this.playAreaCenterOffset = this.PLAY_AREA_OVERLAY_OFFSET;
        this.isPlayAreaVisible = false;
        this.wasPlayAreaVisible = false;
        this.isPlayAreaAvailable = false;
        this.targetOverlayID = null;
        this.playAreaOverlay = null;
        this.playAreaSensorPositionOverlays = [];

        this.TELEPORT_SCALE_DURATION = 130;
        this.TELEPORT_SCALE_TIMEOUT = 25;
        this.isTeleportVisible = false;
        this.teleportScaleTimer = null;
        this.teleportScaleStart = 0;
        this.teleportScaleFactor = 0;
        this.teleportScaleMode = "head";

        this.TELEPORTED_FADE_DELAY_DURATION = 900;
        this.TELEPORTED_FADE_DURATION = 200;
        this.TELEPORTED_FADE_INTERVAL = 25;
        this.TELEPORTED_FADE_DELAY_DELTA = this.TELEPORTED_FADE_INTERVAL / this.TELEPORTED_FADE_DELAY_DURATION;
        this.TELEPORTED_FADE_DELTA = this.TELEPORTED_FADE_INTERVAL / this.TELEPORTED_FADE_DURATION;
        this.teleportedFadeTimer = null;
        this.teleportedFadeDelayFactor = 0;
        this.teleportedFadeFactor = 0;
        this.teleportedPosition = Vec3.ZERO;
        this.TELEPORTED_TARGET_ALPHA = 1.0;
        this.TELEPORTED_TARGET_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 180, z: 0 });
        this.teleportedTargetOverlay = null;

        this.setPlayAreaDimensions = function () {
            var avatarScale = MyAvatar.sensorToWorldScale;

            var playAreaOverlayProperties = {
                dimensions:
                    Vec3.multiply(_this.teleportScaleFactor * avatarScale, {
                        x: _this.playArea.width,
                        y: _this.PLAY_AREA_OVERLAY_MODEL_DIMENSIONS.y,
                        z: _this.playArea.height
                    })
            };

            if (_this.teleportScaleFactor < 1) {
                // Adjust position of playAreOverlay so that its base is at correct height.
                // Always parenting to teleport target is good enough for this.
                var sensorToWorldMatrix = MyAvatar.sensorToWorldMatrix;
                var sensorToWorldRotation = Mat4.extractRotation(MyAvatar.sensorToWorldMatrix);
                var worldToSensorMatrix = Mat4.inverse(sensorToWorldMatrix);
                var avatarSensorPosition = Mat4.transformPoint(worldToSensorMatrix, MyAvatar.position);
                avatarSensorPosition.y = 0;

                var targetRotation = Overlays.getProperty(_this.targetOverlayID, "rotation");
                var relativePlayAreaCenterOffset =
                    Vec3.sum(_this.playAreaCenterOffset, { x: 0, y: -TARGET_MODEL_DIMENSIONS.y / 2, z: 0 });
                var localPosition = Vec3.multiplyQbyV(Quat.inverse(targetRotation),
                    Vec3.multiplyQbyV(sensorToWorldRotation,
                        Vec3.multiply(avatarScale, Vec3.subtract(relativePlayAreaCenterOffset, avatarSensorPosition))));
                localPosition.y = _this.teleportScaleFactor * localPosition.y;

                playAreaOverlayProperties.parentID = _this.targetOverlayID;
                playAreaOverlayProperties.localPosition = localPosition;
            }

            Overlays.editOverlay(_this.playAreaOverlay, playAreaOverlayProperties);

            for (var i = 0; i < _this.playAreaSensorPositionOverlays.length; i++) {
                localPosition = _this.playAreaSensorPositions[i];
                localPosition = Vec3.multiply(avatarScale, localPosition);
                // Position relative to the play area.
                localPosition.y = avatarScale * (_this.PLAY_AREA_SENSOR_OVERLAY_DIMENSIONS.y / 2
                    - _this.PLAY_AREA_OVERLAY_MODEL_DIMENSIONS.y / 2);
                Overlays.editOverlay(_this.playAreaSensorPositionOverlays[i], {
                    dimensions: Vec3.multiply(_this.teleportScaleFactor * avatarScale, _this.PLAY_AREA_SENSOR_OVERLAY_DIMENSIONS),
                    parentID: _this.playAreaOverlay,
                    localPosition: localPosition
                });
            }
        };

        this.updatePlayAreaScale = function () {
            if (_this.isPlayAreaAvailable) {
                _this.setPlayAreaDimensions();
            }
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

        this.addToSelectedItemsList = function (properties) {
            for (var i = 0, length = teleportRenderStates.length; i < length; i++) {
                var state = properties.renderStates[teleportRenderStates[i].name];
                if (state && state.end) {
                    Selection.addToSelectedItemsList(_this.teleporterSelectionName, "overlay", state.end);
                }
            }
        };


        this.cleanup = function() {
            Selection.removeListFromMap(_this.teleporterSelectionName);
            Pointers.removePointer(_this.teleportParabolaHandVisuals);
            Pointers.removePointer(_this.teleportParabolaHandCollisions);
            Pointers.removePointer(_this.teleportParabolaHeadVisuals);
            Pointers.removePointer(_this.teleportParabolaHeadCollisions);
            Picks.removePick(_this.teleportHandCollisionPick);
            Picks.removePick(_this.teleportHeadCollisionPick);
            Overlays.deleteOverlay(_this.teleportedTargetOverlay);
            Overlays.deleteOverlay(_this.playAreaOverlay);
            for (var i = 0; i < _this.playAreaSensorPositionOverlays.length; i++) {
                Overlays.deleteOverlay(_this.playAreaSensorPositionOverlays[i]);
            }
            _this.playAreaSensorPositionOverlays = [];
        };

        this.initPointers = function() {
            if (_this.init) {
                _this.cleanup();
            }

            _this.teleportParabolaHandVisuals = Pointers.createPointer(PickType.Parabola, {
                joint: (_this.hand === RIGHT_HAND) ? "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" : "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
                dirOffset: { x: 0, y: 1, z: 0.1 },
                posOffset: { x: (_this.hand === RIGHT_HAND) ? 0.03 : -0.03, y: 0.2, z: 0.02 },
                filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
                faceAvatar: true,
                scaleWithParent: true,
                centerEndY: false,
                speed: speed,
                accelerationAxis: accelerationAxis,
                rotateAccelerationWithAvatar: true,
                renderStates: teleportRenderStates,
                defaultRenderStates: teleportDefaultRenderStates,
                maxDistance: 8.0
            });

            _this.teleportParabolaHandCollisions = Pointers.createPointer(PickType.Parabola, {
                joint: (_this.hand === RIGHT_HAND) ? "_CAMERA_RELATIVE_CONTROLLER_RIGHTHAND" : "_CAMERA_RELATIVE_CONTROLLER_LEFTHAND",
                dirOffset: { x: 0, y: 1, z: 0.1 },
                posOffset: { x: (_this.hand === RIGHT_HAND) ? 0.03 : -0.03, y: 0.2, z: 0.02 },
                filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
                faceAvatar: true,
                scaleWithParent: true,
                centerEndY: false,
                speed: speed,
                accelerationAxis: accelerationAxis,
                rotateAccelerationWithAvatar: true,
                renderStates: teleportRenderStates,
                maxDistance: 8.0
            });

            _this.teleportParabolaHeadVisuals = Pointers.createPointer(PickType.Parabola, {
                joint: "Avatar",
                filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
                faceAvatar: true,
                scaleWithParent: true,
                centerEndY: false,
                speed: speed,
                accelerationAxis: accelerationAxis,
                rotateAccelerationWithAvatar: true,
                renderStates: teleportRenderStates,
                defaultRenderStates: teleportDefaultRenderStates,
                maxDistance: 8.0
            });

            _this.teleportParabolaHeadCollisions = Pointers.createPointer(PickType.Parabola, {
                joint: "Avatar",
                filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
                faceAvatar: true,
                scaleWithParent: true,
                centerEndY: false,
                speed: speed,
                accelerationAxis: accelerationAxis,
                rotateAccelerationWithAvatar: true,
                renderStates: teleportRenderStates,
                maxDistance: 8.0
            });

            _this.addToSelectedItemsList(Pointers.getPointerProperties(_this.teleportParabolaHandVisuals));
            _this.addToSelectedItemsList(Pointers.getPointerProperties(_this.teleportParabolaHeadVisuals));


            var capsuleData = MyAvatar.getCollisionCapsule();

            var sensorToWorldScale = MyAvatar.getSensorToWorldScale();

            var diameter = 2.0 * capsuleData.radius / sensorToWorldScale;
            var height = (Vec3.distance(capsuleData.start, capsuleData.end) + diameter) / sensorToWorldScale;
            var capsuleRatio = 5.0 * diameter / height;
            var offset = _this.pickHeightOffset * capsuleRatio;

            _this.teleportHandCollisionPick = Picks.createPick(PickType.Collision, {
                enabled: false,
                parentID: Pointers.getPointerProperties(_this.teleportParabolaHandCollisions).renderStates["collision"].end,
                filter: Picks.PICK_ENTITIES | Picks.PICK_AVATARS,
                shape: {
                    shapeType: "capsule-y",
                    dimensions: {
                        x: diameter,
                        y: height,
                        z: diameter
                    }
                },
                position: { x: 0, y: offset + height * 0.5, z: 0 },
                threshold: _this.capsuleThreshold
            });

            _this.teleportHeadCollisionPick = Picks.createPick(PickType.Collision, {
                enabled: false,
                parentID: Pointers.getPointerProperties(_this.teleportParabolaHeadCollisions).renderStates["collision"].end,
                filter: Picks.PICK_ENTITIES | Picks.PICK_AVATARS,
                shape: {
                    shapeType: "capsule-y",
                    dimensions: {
                        x: diameter,
                        y: height,
                        z: diameter
                    }
                },
                position: { x: 0, y: offset + height * 0.5, z: 0 },
                threshold: _this.capsuleThreshold
            });


            _this.playAreaOverlay = Overlays.addOverlay("model", {
                url: _this.PLAY_AREA_OVERLAY_MODEL,
                drawInFront: false,
                visible: false
            });

            _this.teleportedTargetOverlay = Overlays.addOverlay("model", {
                url: TARGET_MODEL_URL,
                alpha: _this.TELEPORTED_TARGET_ALPHA,
                visible: false
            });

            Selection.addToSelectedItemsList(_this.teleporterSelectionName, "overlay", _this.playAreaOverlay);
            Selection.addToSelectedItemsList(_this.teleporterSelectionName, "overlay", _this.teleportedTargetOverlay);


            _this.playArea = HMD.playArea;
            _this.isPlayAreaAvailable = HMD.active && _this.playArea.width !== 0 && _this.playArea.height !== 0;
            if (_this.isPlayAreaAvailable) {
                _this.playAreaCenterOffset = Vec3.sum({ x: _this.playArea.x, y: 0, z: _this.playArea.y },
                    _this.PLAY_AREA_OVERLAY_OFFSET);
                _this.playAreaSensorPositions = HMD.sensorPositions;

                for (var i = 0; i < _this.playAreaSensorPositions.length; i++) {
                    if (i > _this.playAreaSensorPositionOverlays.length - 1) {
                        var overlay = Overlays.addOverlay("model", {
                            url: _this.PLAY_AREA_SENSOR_OVERLAY_MODEL,
                            dimensions: _this.PLAY_AREA_SENSOR_OVERLAY_DIMENSIONS,
                            parentID: _this.playAreaOverlay,
                            localRotation: _this.PLAY_AREA_SENSOR_OVERLAY_ROTATION,
                            drawInFront: false,
                            visible: false
                        });
                        _this.playAreaSensorPositionOverlays.push(overlay);
                        Selection.addToSelectedItemsList(_this.teleporterSelectionName, "overlay", overlay);
                    }
                }

                _this.setPlayAreaDimensions();
            }

            _this.init = true;
        };
        
        _this.initPointers();


        this.translateXAction = Controller.findAction("TranslateX");
        this.translateYAction = Controller.findAction("TranslateY");
        this.translateZAction = Controller.findAction("TranslateZ");

        this.setPlayAreaVisible = function (visible, targetOverlayID, fade) {
            if (!_this.isPlayAreaAvailable || _this.isPlayAreaVisible === visible) {
                return;
            }

            _this.wasPlayAreaVisible = _this.isPlayAreaVisible;
            _this.isPlayAreaVisible = visible;
            _this.targetOverlayID = targetOverlayID;

            if (_this.teleportedFadeTimer !== null) {
                Script.clearTimeout(_this.teleportedFadeTimer);
                _this.teleportedFadeTimer = null;
            }
            if (visible || !fade) {
                // Immediately make visible or invisible.
                _this.isPlayAreaVisible = visible;
                Overlays.editOverlay(_this.playAreaOverlay, {
                    dimensions: Vec3.ZERO,
                    alpha: _this.PLAY_AREA_BOX_ALPHA,
                    visible: visible
                });
                for (var i = 0; i < _this.playAreaSensorPositionOverlays.length; i++) {
                    Overlays.editOverlay(_this.playAreaSensorPositionOverlays[i], {
                        dimensions: Vec3.ZERO,
                        alpha: _this.PLAY_AREA_SENSOR_ALPHA,
                        visible: visible
                    });
                }
                Overlays.editOverlay(_this.teleportedTargetOverlay, { visible: false });
            } else {
                // Fading out of overlays is initiated in setTeleportVisible().
            }
        };

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
                Overlays.editOverlay(_this.playAreaOverlay, {
                    parentID: Uuid.NULL,
                    position: Vec3.sum(position,
                        Vec3.multiplyQbyV(sensorToWorldRotation,
                            Vec3.multiply(MyAvatar.sensorToWorldScale,
                                Vec3.subtract(_this.playAreaCenterOffset, avatarSensorPosition)))),
                    rotation: sensorToWorldRotation
                });
            } else {
                // Set play area position and rotation in local coordinates with parenting.
                var targetRotation = Overlays.getProperty(_this.targetOverlayID, "rotation");
                var sensorToTargetRotation = Quat.multiply(Quat.inverse(targetRotation), sensorToWorldRotation);
                var relativePlayAreaCenterOffset =
                    Vec3.sum(_this.playAreaCenterOffset, { x: 0, y: -TARGET_MODEL_DIMENSIONS.y / 2, z: 0 });
                Overlays.editOverlay(_this.playAreaOverlay, {
                    parentID: _this.targetOverlayID,
                    localPosition: Vec3.multiplyQbyV(Quat.inverse(targetRotation),
                        Vec3.multiplyQbyV(sensorToWorldRotation,
                            Vec3.multiply(MyAvatar.sensorToWorldScale,
                                Vec3.subtract(relativePlayAreaCenterOffset, avatarSensorPosition)))),
                    localRotation: sensorToTargetRotation
                });
            }
        };


        this.scaleInTeleport = function () {
            _this.teleportScaleFactor = Math.min((Date.now() - _this.teleportScaleStart) / _this.TELEPORT_SCALE_DURATION, 1);
            Pointers.editRenderState(
                _this.teleportScaleMode === "head" ? _this.teleportParabolaHeadVisuals : _this.teleportParabolaHandVisuals,
                "teleport",
                {
                    path: teleportPath, // Teleport beam disappears if not included.
                    end: { dimensions: Vec3.multiply(_this.teleportScaleFactor, TARGET_MODEL_DIMENSIONS) }
                }
            );
            if (_this.isPlayAreaVisible) {
                _this.setPlayAreaDimensions();
            }
            if (_this.teleportScaleFactor < 1) {
                _this.teleportScaleTimer = Script.setTimeout(_this.scaleInTeleport, _this.TELEPORT_SCALE_TIMEOUT);
            } else {
                _this.teleportScaleTimer = null;
            }
        };

        this.fadeOutTeleport = function () {
            var isAvatarMoving,
                i, length;

            isAvatarMoving = Controller.getActionValue(_this.translateXAction) !== 0
                || Controller.getActionValue(_this.translateYAction) !== 0
                || Controller.getActionValue(_this.translateZAction) !== 0;

            if (_this.teleportedFadeDelayFactor > 0 && !_this.isTeleportVisible && !isAvatarMoving) {
                // Delay fade.
                _this.teleportedFadeDelayFactor = _this.teleportedFadeDelayFactor - _this.TELEPORTED_FADE_DELAY_DELTA;
                _this.teleportedFadeTimer = Script.setTimeout(_this.fadeOutTeleport, _this.TELEPORTED_FADE_INTERVAL);
            } else if (_this.teleportedFadeFactor > 0 && !_this.isTeleportVisible && !isAvatarMoving) {
                // Fade.
                _this.teleportedFadeFactor = _this.teleportedFadeFactor - _this.TELEPORTED_FADE_DELTA;
                Overlays.editOverlay(_this.teleportedTargetOverlay, {
                    alpha: _this.teleportedFadeFactor * _this.TELEPORTED_TARGET_ALPHA
                });
                if (_this.wasPlayAreaVisible) {
                    Overlays.editOverlay(_this.playAreaOverlay, {
                        alpha: _this.teleportedFadeFactor * _this.PLAY_AREA_BOX_ALPHA
                    });
                    var sensorAlpha = _this.teleportedFadeFactor * _this.PLAY_AREA_SENSOR_ALPHA;
                    for (i = 0, length = _this.playAreaSensorPositionOverlays.length; i < length; i++) {
                        Overlays.editOverlay(_this.playAreaSensorPositionOverlays[i], { alpha: sensorAlpha });
                    }
                }
                _this.teleportedFadeTimer = Script.setTimeout(_this.fadeOutTeleport, _this.TELEPORTED_FADE_INTERVAL);
            } else {
                // Make invisible.
                Overlays.editOverlay(_this.teleportedTargetOverlay, { visible: false });
                if (_this.wasPlayAreaVisible) {
                    Overlays.editOverlay(_this.playAreaOverlay, { visible: false });
                    for (i = 0, length = _this.playAreaSensorPositionOverlays.length; i < length; i++) {
                        Overlays.editOverlay(_this.playAreaSensorPositionOverlays[i], { visible: false });
                    }
                }
                _this.teleportedFadeTimer = null;
                Selection.disableListHighlight(_this.teleporterSelectionName);
            }
        };

        this.cancelFade = function () {
            // Other hand may call this to immediately hide fading overlays.
            var i, length;
            if (_this.teleportedFadeTimer) {
                Overlays.editOverlay(_this.teleportedTargetOverlay, { visible: false });
                if (_this.wasPlayAreaVisible) {
                    Overlays.editOverlay(_this.playAreaOverlay, { visible: false });
                    for (i = 0, length = _this.playAreaSensorPositionOverlays.length; i < length; i++) {
                        Overlays.editOverlay(_this.playAreaSensorPositionOverlays[i], { visible: false });
                    }
                }
                _this.teleportedFadeTimer = null;
            }
        };

        this.setTeleportVisible = function (visible, mode, fade) {
            // Scales in teleport target and play area when start displaying them.
            if (visible === _this.isTeleportVisible) {
                return;
            }

            if (visible) {
                _this.teleportScaleMode = mode;
                Pointers.editRenderState(
                    mode === "head" ? _this.teleportParabolaHeadVisuals : _this.teleportParabolaHandVisuals,
                    "teleport",
                    {
                        path: teleportPath, // Teleport beam disappears if not included.
                        end: { dimensions: Vec3.ZERO }
                    }
                );
                _this.getOtherModule().cancelFade();
                _this.teleportScaleStart = Date.now();
                _this.teleportScaleFactor = 0;
                _this.scaleInTeleport();
                Selection.enableListHighlight(_this.teleporterSelectionName, _this.TELEPORTER_SELECTION_STYLE);
            } else {
                if (_this.teleportScaleTimer !== null) {
                    Script.clearTimeout(_this.teleportScaleTimer);
                    _this.teleportScaleTimer = null;
                }

                if (fade) {
                    // Copy of target at teleported position for fading.
                    var avatarScale = MyAvatar.sensorToWorldScale;
                    Overlays.editOverlay(_this.teleportedTargetOverlay, {
                        position: Vec3.sum(_this.teleportedPosition, {
                            x: 0,
                            y: -getAvatarFootOffset() + avatarScale * TARGET_MODEL_DIMENSIONS.y / 2,
                            z: 0
                        }),
                        rotation: Quat.multiply(_this.TELEPORTED_TARGET_ROTATION, MyAvatar.orientation),
                        dimensions: Vec3.multiply(avatarScale, TARGET_MODEL_DIMENSIONS),
                        alpha: _this.TELEPORTED_TARGET_ALPHA,
                        visible: true
                    });

                    // Fade out over time.
                    _this.teleportedFadeDelayFactor = 1.0;
                    _this.teleportedFadeFactor = 1.0;
                    _this.teleportedFadeTimer = Script.setTimeout(_this.fadeOutTeleport, _this.TELEPORTED_FADE_DELAY);
                } else {
                    Selection.disableListHighlight(_this.teleporterSelectionName);
                }
            }

            _this.isTeleportVisible = visible;
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
            return Controller.getValue(Controller.Hardware.Application.AdvancedMovement) &&
                (_this.axisButtonStateX !== 0 || _this.axisButtonStateY !== 0);
        };

        this.buttonPress = function (value) {
            // Button press on gamepad.
            if (value === 0 || !_this.teleportLocked()) {
                _this.buttonValue = value;
                _this.buttonTeleporting = true;
            }
        };

        this.getStandardLY = function (value) {
            _this.standardAxisLY = value;
        };

        this.getStandardRY = function (value) {
            _this.standardAxisRY = value;
        };

        // Return value for the getDominantY and getOffhandY functions has to be inverted.
        this.getDominantY = function () {
            return (MyAvatar.getDominantHand() === "left") ? -(_this.standardAxisLY) : -(_this.standardAxisRY);
        };

        this.getOffhandY = function () {
            return (MyAvatar.getDominantHand() === "left") ? -(_this.standardAxisRY) : -(_this.standardAxisLY);
        };

        this.getDominantHand = function () {
            return (MyAvatar.getDominantHand() === "left") ? LEFT_HAND : RIGHT_HAND;
        };

        this.getOffHand = function () {
            return (MyAvatar.getDominantHand() === "left") ? RIGHT_HAND : LEFT_HAND;
        };

        this.showReticle = function () {
            return (_this.getDominantY() > TELEPORT_DEADZONE) ? true : false;
        };

        this.shouldTeleport = function () {
            return ((_this.getDominantY() > TELEPORT_DEADZONE && _this.getOffhandY() > TELEPORT_DEADZONE)
                || (_this.buttonTeleporting && _this.buttonValue === 0)) ? true : false;
        };

        this.shouldCancel = function () {
            return (_this.getDominantY() <= TELEPORT_DEADZONE && !_this.buttonTeleporting) ? true : false;
        };

        this.parameters = makeDispatcherModuleParameters(
            80,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.enterTeleport = function() {
            _this.state = TELEPORTER_STATES.TARGETTING;
        };

        this.isReady = function(controllerData, deltaTime) {
            if ((Window.interstitialModeEnabled && !Window.isPhysicsEnabled()) || !MyAvatar.allowTeleporting) {
                return makeRunningValues(false, [], []);
            }

            var otherModule = this.getOtherModule();
            if (!this.disabled && !otherModule.active && (this.showReticle() && this.hand === this.getDominantHand()
                    || this.buttonValue !== 0)) {
                this.active = true;
                this.enterTeleport();
                return makeRunningValues(true, [], []);
            }
            return makeRunningValues(false, [], []);
        };

        this.run = function(controllerData, deltaTime) {
            // Kill condition:
            if (_this.shouldCancel()) {
                _this.disableLasers();
                this.active = false;
                return makeRunningValues(false, [], []);
            }

            // Get current hand pose information to see if the pose is valid
            var pose = Controller.getPoseValue(handInfo[(_this.hand === RIGHT_HAND) ? 'right' : 'left'].controllerInput);
            var mode = pose.valid ? _this.hand : 'head';
            if (!pose.valid) {
                Pointers.disablePointer(_this.teleportParabolaHandVisuals);
                Pointers.disablePointer(_this.teleportParabolaHandCollisions);
                Picks.disablePick(_this.teleportHandCollisionPick);
                Pointers.enablePointer(_this.teleportParabolaHeadVisuals);
                Pointers.enablePointer(_this.teleportParabolaHeadCollisions);
                Picks.enablePick(_this.teleportHeadCollisionPick);
            } else {
                Pointers.enablePointer(_this.teleportParabolaHandVisuals);
                Pointers.enablePointer(_this.teleportParabolaHandCollisions);
                Picks.enablePick(_this.teleportHandCollisionPick);
                Pointers.disablePointer(_this.teleportParabolaHeadVisuals);
                Pointers.disablePointer(_this.teleportParabolaHeadCollisions);
                Picks.disablePick(_this.teleportHeadCollisionPick);
            }

            // We do up to 2 picks to find a teleport location.
            // There are 2 types of teleport locations we are interested in:
            //
            //   1. A visible floor. This can be any entity surface that points within some degree of "up" 
            //      and where the avatar capsule can be positioned without colliding
            //
            //   2. A seat. The seat can be visible or invisible.
            //
            //  The Collision Pick is currently parented to the end overlay on teleportParabolaXXXXCollisions
            //
            //  TODO 
            //  Parent the collision Pick directly to the teleportParabolaXXXXVisuals and get rid of teleportParabolaXXXXCollisions
            //
            var result, collisionResult;
            if (mode === 'head') {
                result = Pointers.getPrevPickResult(_this.teleportParabolaHeadCollisions);
                collisionResult = Picks.getPrevPickResult(_this.teleportHeadCollisionPick);
            } else {
                result = Pointers.getPrevPickResult(_this.teleportParabolaHandCollisions);
                collisionResult = Picks.getPrevPickResult(_this.teleportHandCollisionPick);
            }
           
            var teleportLocationType = getTeleportTargetType(result, collisionResult);

            if (teleportLocationType === TARGET.NONE) {
                // Use the cancel default state
                _this.setTeleportState(mode, "cancel", "");
            } else if (teleportLocationType === TARGET.INVALID) {
                _this.setTeleportState(mode, "", "cancel");
            } else if (teleportLocationType === TARGET.COLLIDES) {
                _this.setTeleportState(mode, "cancel", "collision");
            } else if (teleportLocationType === TARGET.SURFACE || teleportLocationType === TARGET.DISCREPANCY) {
                _this.setTeleportState(mode, "teleport", "collision");
                _this.updatePlayArea(result.intersection);
            } else if (teleportLocationType === TARGET.SEAT) {
                _this.setTeleportState(mode, "collision", "seat");
            }
            return _this.teleport(result, teleportLocationType);
        };

        this.teleport = function(newResult, target) {
            var result = newResult;
            _this.teleportedPosition = newResult.intersection;
            if (!_this.shouldTeleport()) {
                return makeRunningValues(true, [], []);
            }

            if (target === TARGET.NONE || target === TARGET.INVALID) {
                // Do nothing
            } else if (target === TARGET.SEAT) {
                Entities.callEntityMethod(result.objectID, 'sit');
            } else if (target === TARGET.SURFACE || target === TARGET.DISCREPANCY) {
                var offset = getAvatarFootOffset();
                result.intersection.y += offset;
                var shouldLandSafe = target === TARGET.DISCREPANCY;
                MyAvatar.goToLocation(result.intersection, true, HMD.orientation, false, shouldLandSafe);
                HMD.centerUI();
                MyAvatar.centerBody();
            }

            _this.disableLasers();
            _this.active = false;
            _this.buttonTeleporting = false;
            return makeRunningValues(false, [], []);
        };

        this.disableLasers = function() {
            _this.setPlayAreaVisible(false, null, false);
            _this.setTeleportVisible(false, null, false);
            Pointers.disablePointer(_this.teleportParabolaHandVisuals);
            Pointers.disablePointer(_this.teleportParabolaHandCollisions);
            Pointers.disablePointer(_this.teleportParabolaHeadVisuals);
            Pointers.disablePointer(_this.teleportParabolaHeadCollisions);
            Picks.disablePick(_this.teleportHeadCollisionPick);
            Picks.disablePick(_this.teleportHandCollisionPick);
        };

        this.teleportState = "";

        this.setTeleportState = function (mode, visibleState, invisibleState) {
            var teleportState = mode + visibleState + invisibleState;
            if (teleportState === _this.teleportState) {
                return;
            }
            _this.teleportState = teleportState;

            var pointerID;
            if (mode === 'head') {
                Pointers.setRenderState(_this.teleportParabolaHeadVisuals, visibleState);
                Pointers.setRenderState(_this.teleportParabolaHeadCollisions, invisibleState);
                pointerID = _this.teleportParabolaHeadVisuals;
            } else {
                Pointers.setRenderState(_this.teleportParabolaHandVisuals, visibleState);
                Pointers.setRenderState(_this.teleportParabolaHandCollisions, invisibleState);
                pointerID = _this.teleportParabolaHandVisuals;
            }
            var visible = visibleState === "teleport";
            _this.setPlayAreaVisible(visible && MyAvatar.showPlayArea,
                Pointers.getPointerProperties(pointerID).renderStates.teleport.end, false);
            _this.setTeleportVisible(visible, mode, false);
        };

        this.setIgnoreEntities = function(entitiesToIgnore) {
            Pointers.setIgnoreItems(_this.teleportParabolaHandVisuals, entitiesToIgnore);
            Pointers.setIgnoreItems(_this.teleportParabolaHandCollisions, entitiesToIgnore);
            Pointers.setIgnoreItems(_this.teleportParabolaHeadVisuals, entitiesToIgnore);
            Pointers.setIgnoreItems(_this.teleportParabolaHeadCollisions, entitiesToIgnore);
            Picks.setIgnoreItems(_this.teleportHeadCollisionPick, entitiesToIgnore);
            Picks.setIgnoreItems(_this.teleportHandCollisionPick, entitiesToIgnore);
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
    var isGamePadMapped = false;

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
    var MAX_DISCREPANCY_DISTANCE = 1.0;
    var MAX_DOT_SIGN = -0.6;

    function checkForMeshDiscrepancy(result, collisionResult) {
        var intersectingObjects = collisionResult.intersectingObjects;
        if (intersectingObjects.length > 0 && intersectingObjects.length < 3) {
            for (var j = 0; j < collisionResult.intersectingObjects.length; j++) {
                var intersectingObject = collisionResult.intersectingObjects[j];
                for (var i = 0; i < intersectingObject.collisionContacts.length; i++) {
                    var normal = intersectingObject.collisionContacts[i].normalOnPick;
                    var distanceToPick = Vec3.distance(intersectingObject.collisionContacts[i].pointOnPick, result.intersection);
                    var normalSign = Vec3.dot(normal, Quat.getUp(MyAvatar.orientation));
                    if ((distanceToPick > MAX_DISCREPANCY_DISTANCE) || (normalSign > MAX_DOT_SIGN)) {
                        return false;
                    }
                }
            }
            return true;
        }
        return false;
    }
    
    function getTeleportTargetType(result, collisionResult) {
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
        var isDiscrepancy = false;
        if (collisionResult.collisionRegion != undefined) {
            if (collisionResult.intersects) {
                isDiscrepancy = checkForMeshDiscrepancy(result, collisionResult);
                if (!isDiscrepancy) {
                    return TARGET.COLLIDES;
                } 
            }
        }

        var surfaceNormal = result.surfaceNormal;
        var angle = Math.acos(Vec3.dot(surfaceNormal, Quat.getUp(MyAvatar.orientation))) * (180.0 / Math.PI);

        if (angle > MAX_ANGLE_FROM_UP_TO_TELEPORT) {
            return TARGET.INVALID;
        } else if (isDiscrepancy) {
            return TARGET.DISCREPANCY;
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

    function registerGamePadMapping(teleporter) {
        if (Controller.Hardware.GamePad) {
            var mappingName = 'Hifi-Teleporter-Dev-GamePad-' + Math.random();
            var teleportMapping = Controller.newMapping(mappingName);
            teleportMapping.from(Controller.Hardware.GamePad.Y).peek().to(rightTeleporter.buttonPress);
            Controller.enableMapping(mappingName);
            isGamePadMapped = true;
        }
    }

    function onHardwareChanged() {
        // Controller.Hardware.Vive is not immediately available at Interface start-up.
        if (!isViveMapped && Controller.Hardware.Vive) {
            registerViveTeleportMapping();
        }

        if (!isGamePadMapped && Controller.Hardware.GamePad) {
            registerGamePadMapping();
        }
    }

    Controller.hardwareChanged.connect(onHardwareChanged);

    function registerMappings() {
        mappingName = 'Hifi-Teleporter-Dev-' + Math.random();
        teleportMapping = Controller.newMapping(mappingName);

        // Vive teleport button lock-out.
        registerViveTeleportMapping();

        // Gamepad "Y" button teleport.
        registerGamePadMapping();

        // Teleport actions.
        teleportMapping.from(Controller.Standard.LY).peek().to(leftTeleporter.getStandardLY);
        teleportMapping.from(Controller.Standard.RY).peek().to(leftTeleporter.getStandardRY);
        teleportMapping.from(Controller.Standard.LY).peek().to(rightTeleporter.getStandardLY);
        teleportMapping.from(Controller.Standard.RY).peek().to(rightTeleporter.getStandardRY);
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
    
    MyAvatar.onLoadComplete.connect(function () {
        Script.setTimeout(function () {
            leftTeleporter.initPointers();
            rightTeleporter.initPointers();
        }, 500);
    });
    
    Messages.subscribe('Hifi-Teleport-Disabler');
    Messages.subscribe('Hifi-Teleport-Ignore-Add');
    Messages.subscribe('Hifi-Teleport-Ignore-Remove');
    Messages.messageReceived.connect(handleTeleportMessages);

    MyAvatar.sensorToWorldScaleChanged.connect(function () {
        leftTeleporter.updatePlayAreaScale();
        rightTeleporter.updatePlayAreaScale();
    });

}()); // END LOCAL_SCOPE
