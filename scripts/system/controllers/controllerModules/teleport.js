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

    var TARGET_MODEL_URL = Script.resolvePath("../../assets/models/teleport-destination.fbx");
    var SEAT_MODEL_URL = Script.resolvePath("../../assets/models/teleport-seat.fbx");

    var TARGET_MODEL_DIMENSIONS = {
        x: 1.15,
        y: 0.5,
        z: 1.15
    };

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
        width: 0.025
    };
    
    var teleportPath = {
        color: COLORS_TELEPORT_CAN_TELEPORT,
        alpha: 1,
        width: 0.025
    };
    
    var seatPath = {
        color: COLORS_TELEPORT_SEAT,
        alpha: 1,
        width: 0.025
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

        this.cleanup = function() {
            Pointers.removePointer(_this.teleportParabolaHandVisuals);
            Pointers.removePointer(_this.teleportParabolaHandCollisions);
            Pointers.removePointer(_this.teleportParabolaHeadVisuals);
            Pointers.removePointer(_this.teleportParabolaHeadCollisions);
            Picks.removePick(_this.teleportHandCollisionPick);
            Picks.removePick(_this.teleportHeadCollisionPick);
        };

        this.initPointers = function () {
            if (_this.init) {
                _this.cleanup();
            }
            _this.teleportParabolaHandVisuals = Pointers.createPointer(PickType.Parabola, {
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
                defaultRenderStates: teleportDefaultRenderStates,
                maxDistance: 8.0
            });

            _this.teleportParabolaHandCollisions = Pointers.createPointer(PickType.Parabola, {
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

            _this.teleportParabolaHeadVisuals = Pointers.createPointer(PickType.Parabola, {
                joint: "Avatar",
                filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
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

            _this.teleportParabolaHeadCollisions = Pointers.createPointer(PickType.Parabola, {
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


            var capsuleData = MyAvatar.getCollisionCapsule();

            var sensorToWorldScale = MyAvatar.getSensorToWorldScale();

            var radius = capsuleData.radius / sensorToWorldScale;
            var height = (Vec3.distance(capsuleData.start, capsuleData.end) + (capsuleData.radius * 2.0)) / sensorToWorldScale;
            var capsuleRatio = 10.0 * radius / height;
            var offset = _this.pickHeightOffset * capsuleRatio;

            _this.teleportHandCollisionPick = Picks.createPick(PickType.Collision, {
                enabled: true,
                parentID: Pointers.getPointerProperties(_this.teleportParabolaHandCollisions).renderStates["collision"].end,
                filter: Picks.PICK_ENTITIES | Picks.PICK_AVATARS,
                shape: {
                    shapeType: "capsule-y",
                    dimensions: {
                        x: radius * 2.0,
                        y: height - (radius * 2.0),
                        z: radius * 2.0
                    }
                },
                position: { x: 0, y: offset + height * 0.5, z: 0 },
                threshold: _this.capsuleThreshold
            });

            _this.teleportHeadCollisionPick = Picks.createPick(PickType.Collision, {
                enabled: true,
                parentID: Pointers.getPointerProperties(_this.teleportParabolaHeadCollisions).renderStates["collision"].end,
                filter: Picks.PICK_ENTITIES | Picks.PICK_AVATARS,
                shape: {
                    shapeType: "capsule-y",
                    dimensions: {
                        x: radius * 2.0,
                        y: height - (radius * 2.0),
                        z: radius * 2.0
                    }
                },
                position: { x: 0, y: offset + height * 0.5, z: 0 },
                threshold: _this.capsuleThreshold
            });
            _this.init = true;
        }
        
        _this.initPointers();

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
                this.setTeleportState(mode, "cancel", "");
            } else if (teleportLocationType === TARGET.INVALID) {
                this.setTeleportState(mode, "", "cancel");
            } else if (teleportLocationType === TARGET.COLLIDES) {
                this.setTeleportState(mode, "cancel", "collision");
            } else if (teleportLocationType === TARGET.SURFACE || teleportLocationType === TARGET.DISCREPANCY) {
                this.setTeleportState(mode, "teleport", "collision");
            } else if (teleportLocationType === TARGET.SEAT) {
                this.setTeleportState(mode, "collision", "seat");
            }
            return this.teleport(result, teleportLocationType);
        };

        this.teleport = function(newResult, target) {
            var result = newResult;
            if (_this.buttonValue !== 0) {
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

            this.disableLasers();
            this.active = false;
            return makeRunningValues(false, [], []);
        };

        this.disableLasers = function() {
            Pointers.disablePointer(_this.teleportParabolaHandVisuals);
            Pointers.disablePointer(_this.teleportParabolaHandCollisions);
            Pointers.disablePointer(_this.teleportParabolaHeadVisuals);
            Pointers.disablePointer(_this.teleportParabolaHeadCollisions);
            Picks.disablePick(_this.teleportHeadCollisionPick);
            Picks.disablePick(_this.teleportHandCollisionPick);
        };

        this.setTeleportState = function(mode, visibleState, invisibleState) {
            if (mode === 'head') {
                Pointers.setRenderState(_this.teleportParabolaHeadVisuals, visibleState);
                Pointers.setRenderState(_this.teleportParabolaHeadCollisions, invisibleState);
            } else {
                Pointers.setRenderState(_this.teleportParabolaHandVisuals, visibleState);
                Pointers.setRenderState(_this.teleportParabolaHandCollisions, invisibleState);
            }
        };

        this.setIgnoreEntities = function(entitiesToIgnore) {
            Pointers.setIgnoreItems(this.teleportParabolaHandVisuals, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportParabolaHandCollisions, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportParabolaHeadVisuals, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportParabolaHeadCollisions, entitiesToIgnore);
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

}()); // END LOCAL_SCOPE
