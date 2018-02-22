"use strict";

// Created by james b. pollack @imgntn on 7/2/2016
// Copyright 2016 High Fidelity, Inc.
//
//  Creates a beam and target and then teleports you there.  Release when its close to you to cancel.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* jslint bitwise: true */

/* global Script, Entities, MyAvatar, Controller, RIGHT_HAND, LEFT_HAND, getControllerJointIndex,
   enableDispatcherModule, disableDispatcherModule, Messages, makeDispatcherModuleParameters, makeRunningValues, Vec3,
   RayPick, HMD, Uuid, AvatarList, Picks, Pointers, PickType
*/

Script.include("/~/system/libraries/Xform.js");
Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() { // BEGIN LOCAL_SCOPE

    var TARGET_MODEL_URL = Script.resolvePath("../../assets/models/teleport-destination.fbx");
    var TOO_CLOSE_MODEL_URL = Script.resolvePath("../../assets/models/teleport-cancel.fbx");
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

    var TELEPORT_CANCEL_RANGE = 1;
    var COOL_IN_DURATION = 300;

    var handInfo = {
        right: {
            controllerInput: Controller.Standard.RightHand
        },
        left: {
            controllerInput: Controller.Standard.LeftHand
        }
    };

    var cancelPath = {
        type: "line3d",
        color: COLORS_TELEPORT_CANCEL,
        ignoreRayIntersection: true,
        alpha: 1,
        solid: true,
        drawInFront: true,
        glow: 1.0
    };
    var teleportPath = {
        type: "line3d",
        color: COLORS_TELEPORT_CAN_TELEPORT,
        ignoreRayIntersection: true,
        alpha: 1,
        solid: true,
        drawInFront: true,
        glow: 1.0
    };
    var seatPath = {
        type: "line3d",
        color: COLORS_TELEPORT_SEAT,
        ignoreRayIntersection: true,
        alpha: 1,
        solid: true,
        drawInFront: true,
        glow: 1.0
    };
    var cancelEnd = {
        type: "model",
        url: TOO_CLOSE_MODEL_URL,
        dimensions: TARGET_MODEL_DIMENSIONS,
        ignoreRayIntersection: true
    };
    var teleportEnd = {
        type: "model",
        url: TARGET_MODEL_URL,
        dimensions: TARGET_MODEL_DIMENSIONS,
        ignoreRayIntersection: true
    };
    var seatEnd = {
        type: "model",
        url: SEAT_MODEL_URL,
        dimensions: TARGET_MODEL_DIMENSIONS,
        ignoreRayIntersection: true
    };


    var teleportRenderStates = [{name: "cancel", path: cancelPath, end: cancelEnd},
        {name: "teleport", path: teleportPath, end: teleportEnd},
        {name: "seat", path: seatPath, end: seatEnd}];

    var DEFAULT_DISTANCE = 50;
    var teleportDefaultRenderStates = [{name: "cancel", distance: DEFAULT_DISTANCE, path: cancelPath}];

    var coolInTimeout = null;
    var ignoredEntities = [];


    var TELEPORTER_STATES = {
        IDLE: 'idle',
        COOL_IN: 'cool_in',
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

        this.teleportRayHandVisible = Pointers.createPointer(PickType.Ray, {
            joint: (_this.hand === RIGHT_HAND) ? "RightHand" : "LeftHand",
            filter: Picks.PICK_ENTITIES,
            faceAvatar: true,
            scaleWithAvatar: true,
            centerEndY: false,
            renderStates: teleportRenderStates,
            defaultRenderStates: teleportDefaultRenderStates
        });
        this.teleportRayHandInvisible = Pointers.createPointer(PickType.Ray, {
            joint: (_this.hand === RIGHT_HAND) ? "RightHand" : "LeftHand",
            filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
            faceAvatar: true,
            scaleWithAvatar: true,
            centerEndY: false,
            renderStates: teleportRenderStates
        });
        this.teleportRayHeadVisible = Pointers.createPointer(PickType.Ray, {
            joint: "Avatar",
            filter: Picks.PICK_ENTITIES,
            faceAvatar: true,
            scaleWithAvatar: true,
            centerEndY: false,
            renderStates: teleportRenderStates,
            defaultRenderStates: teleportDefaultRenderStates
        });
        this.teleportRayHeadInvisible = Pointers.createPointer(PickType.Ray, {
            joint: "Avatar",
            filter: Picks.PICK_ENTITIES | Picks.PICK_INCLUDE_INVISIBLE,
            faceAvatar: true,
            scaleWithAvatar: true,
            centerEndY: false,
            renderStates: teleportRenderStates
        });

        this.cleanup = function() {
            Pointers.removePointer(this.teleportRayHandVisible);
            Pointers.removePointer(this.teleportRayHandInvisible);
            Pointers.removePointer(this.teleportRayHeadVisible);
            Pointers.removePointer(this.teleportRayHeadInvisible);
        };

        this.buttonPress = function(value) {
            _this.buttonValue = value;
        };

        this.parameters = makeDispatcherModuleParameters(
            80,
            this.hand === RIGHT_HAND ? ["rightHand"] : ["leftHand"],
            [],
            100);

        this.enterTeleport = function() {
            if (coolInTimeout !== null) {
                Script.clearTimeout(coolInTimeout);
            }

            this.state = TELEPORTER_STATES.COOL_IN;
            coolInTimeout = Script.setTimeout(function() {
                if (_this.state === TELEPORTER_STATES.COOL_IN) {
                    _this.state = TELEPORTER_STATES.TARGETTING;
                }
            }, COOL_IN_DURATION);

            // pad scale with avatar size
            var AVATAR_PROPORTIONAL_TARGET_MODEL_DIMENSIONS = Vec3.multiply(MyAvatar.sensorToWorldScale, TARGET_MODEL_DIMENSIONS);

            if (!Vec3.equal(AVATAR_PROPORTIONAL_TARGET_MODEL_DIMENSIONS, cancelEnd.dimensions)) {
                cancelEnd.dimensions = AVATAR_PROPORTIONAL_TARGET_MODEL_DIMENSIONS;
                teleportEnd.dimensions = AVATAR_PROPORTIONAL_TARGET_MODEL_DIMENSIONS;
                seatEnd.dimensions = AVATAR_PROPORTIONAL_TARGET_MODEL_DIMENSIONS;

                teleportRenderStates = [{name: "cancel", path: cancelPath, end: cancelEnd},
                    {name: "teleport", path: teleportPath, end: teleportEnd},
                    {name: "seat", path: seatPath, end: seatEnd}];

                Pointers.editRenderState(this.teleportRayHandVisible, "cancel", teleportRenderStates[0]);
                Pointers.editRenderState(this.teleportRayHandInvisible, "cancel", teleportRenderStates[0]);
                Pointers.editRenderState(this.teleportRayHeadVisible, "cancel", teleportRenderStates[0]);
                Pointers.editRenderState(this.teleportRayHeadInvisible, "cancel", teleportRenderStates[0]);

                Pointers.editRenderState(this.teleportRayHandVisible, "teleport", teleportRenderStates[1]);
                Pointers.editRenderState(this.teleportRayHandInvisible, "teleport", teleportRenderStates[1]);
                Pointers.editRenderState(this.teleportRayHeadVisible, "teleport", teleportRenderStates[1]);
                Pointers.editRenderState(this.teleportRayHeadInvisible, "teleport", teleportRenderStates[1]);

                Pointers.editRenderState(this.teleportRayHandVisible, "seat", teleportRenderStates[2]);
                Pointers.editRenderState(this.teleportRayHandInvisible, "seat", teleportRenderStates[2]);
                Pointers.editRenderState(this.teleportRayHeadVisible, "seat", teleportRenderStates[2]);
                Pointers.editRenderState(this.teleportRayHeadInvisible, "seat", teleportRenderStates[2]);
            }
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
                Pointers.disablePointer(_this.teleportRayHandVisible);
                Pointers.disablePointer(_this.teleportRayHandInvisible);
                Pointers.enablePointer(_this.teleportRayHeadVisible);
                Pointers.enablePointer(_this.teleportRayHeadInvisible);
            } else {
                Pointers.enablePointer(_this.teleportRayHandVisible);
                Pointers.enablePointer(_this.teleportRayHandInvisible);
                Pointers.disablePointer(_this.teleportRayHeadVisible);
                Pointers.disablePointer(_this.teleportRayHeadInvisible);
            }

            // We do up to 2 ray picks to find a teleport location.
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
                result = Pointers.getPrevPickResult(_this.teleportRayHeadInvisible);
            } else {
                result = Pointers.getPrevPickResult(_this.teleportRayHandInvisible);
            }

            var teleportLocationType = getTeleportTargetType(result);
            if (teleportLocationType === TARGET.INVISIBLE) {
                if (mode === 'head') {
                    result = Pointers.getPrevPickResult(_this.teleportRayHeadVisible);
                } else {
                    result = Pointers.getPrevPickResult(_this.teleportRayHandVisible);
                }
                teleportLocationType = getTeleportTargetType(result);
            }

            if (teleportLocationType === TARGET.NONE) {
                // Use the cancel default state
                this.setTeleportState(mode, "cancel", "");
            } else if (teleportLocationType === TARGET.INVALID || teleportLocationType === TARGET.INVISIBLE) {
                this.setTeleportState(mode, "", "cancel");
            } else if (teleportLocationType === TARGET.SURFACE) {
                if (this.state === TELEPORTER_STATES.COOL_IN) {
                    this.setTeleportState(mode, "cancel", "");
                } else {
                    this.setTeleportState(mode, "teleport", "");
                }
            } else if (teleportLocationType === TARGET.SEAT) {
                this.setTeleportState(mode, "", "seat");
            }
            return this.teleport(result, teleportLocationType);
        };

        this.teleport = function(newResult, target) {
            var result = newResult;
            if (_this.buttonValue !== 0) {
                return makeRunningValues(true, [], []);
            }

            if (target === TARGET.NONE || target === TARGET.INVALID || this.state === TELEPORTER_STATES.COOL_IN) {
                // Do nothing
            } else if (target === TARGET.SEAT) {
                Entities.callEntityMethod(result.objectID, 'sit');
            } else if (target === TARGET.SURFACE) {
                var offset = getAvatarFootOffset();
                result.intersection.y += offset;
                MyAvatar.goToLocation(result.intersection, false, {x: 0, y: 0, z: 0, w: 1}, false);
                HMD.centerUI();
                MyAvatar.centerBody();
            }

            this.disableLasers();
            this.active = false;
            return makeRunningValues(false, [], []);
        };

        this.disableLasers = function() {
            Pointers.disablePointer(_this.teleportRayHandVisible);
            Pointers.disablePointer(_this.teleportRayHandInvisible);
            Pointers.disablePointer(_this.teleportRayHeadVisible);
            Pointers.disablePointer(_this.teleportRayHeadInvisible);
        };

        this.setTeleportState = function(mode, visibleState, invisibleState) {
            if (mode === 'head') {
                Pointers.setRenderState(_this.teleportRayHeadVisible, visibleState);
                Pointers.setRenderState(_this.teleportRayHeadInvisible, invisibleState);
            } else {
                Pointers.setRenderState(_this.teleportRayHandVisible, visibleState);
                Pointers.setRenderState(_this.teleportRayHandInvisible, invisibleState);
            }
        };

        this.setIgnoreEntities = function(entitiesToIgnore) {
            Pointers.setIgnoreItems(this.teleportRayHandVisible, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportRayHandInvisible, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportRayHeadVisible, entitiesToIgnore);
            Pointers.setIgnoreItems(this.teleportRayHeadInvisible, entitiesToIgnore);
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

    function parseJSON(json) {
        try {
            return JSON.parse(json);
        } catch (e) {
            return undefined;
        }
    }
    // When determininig whether you can teleport to a location, the normal of the
    // point that is being intersected with is looked at. If this normal is more
    // than MAX_ANGLE_FROM_UP_TO_TELEPORT degrees from <0, 1, 0> (straight up), then
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
        var adj = Math.sqrt(surfaceNormal.x * surfaceNormal.x + surfaceNormal.z * surfaceNormal.z);
        var angleUp = Math.atan2(surfaceNormal.y, adj) * (180 / Math.PI);

        if (angleUp < (90 - MAX_ANGLE_FROM_UP_TO_TELEPORT) ||
            angleUp > (90 + MAX_ANGLE_FROM_UP_TO_TELEPORT) ||
            Vec3.distance(MyAvatar.position, result.intersection) <= TELEPORT_CANCEL_RANGE * MyAvatar.sensorToWorldScale) {
            return TARGET.INVALID;
        } else {
            return TARGET.SURFACE;
        }
    }

    function registerMappings() {
        mappingName = 'Hifi-Teleporter-Dev-' + Math.random();
        teleportMapping = Controller.newMapping(mappingName);

        teleportMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(rightTeleporter.buttonPress);
        teleportMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(leftTeleporter.buttonPress);
    }

    var leftTeleporter = new Teleporter(LEFT_HAND);
    var rightTeleporter = new Teleporter(RIGHT_HAND);

    enableDispatcherModule("LeftTeleporter", leftTeleporter);
    enableDispatcherModule("RightTeleporter", rightTeleporter);
    registerMappings();
    Controller.enableMapping(mappingName);

    function cleanup() {
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

}()); // END LOCAL_SCOPE
