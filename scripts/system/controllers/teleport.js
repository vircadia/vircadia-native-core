"use strict";

// Created by james b. pollack @imgntn on 7/2/2016
// Copyright 2016 High Fidelity, Inc.
//
//  Creates a beam and target and then teleports you there.  Release when its close to you to cancel.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() { // BEGIN LOCAL_SCOPE

var inTeleportMode = false;

var SMOOTH_ARRIVAL_SPACING = 33;
var NUMBER_OF_STEPS = 6;

var TARGET_MODEL_URL = Script.resolvePath("../assets/models/teleport-destination.fbx");
var TOO_CLOSE_MODEL_URL = Script.resolvePath("../assets/models/teleport-cancel.fbx");
var SEAT_MODEL_URL = Script.resolvePath("../assets/models/teleport-seat.fbx");

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
var COOL_IN_DURATION = 500;

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
}

var teleportRenderStates = [{name: "cancel", path: cancelPath, end: cancelEnd},
                            {name: "teleport", path: teleportPath, end: teleportEnd},
                            {name: "seat", path: seatPath, end: seatEnd}];

var DEFAULT_DISTANCE = 50;
var teleportDefaultRenderStates = [{name: "cancel", distance: DEFAULT_DISTANCE, path: cancelPath}];

function ThumbPad(hand) {
    this.hand = hand;
    var _thisPad = this;

    this.buttonPress = function(value) {
        _thisPad.buttonValue = value;
    };
}

function Trigger(hand) {
    this.hand = hand;
    var _this = this;

    this.buttonPress = function(value) {
        _this.buttonValue = value;
    };

    this.down = function() {
        var down = _this.buttonValue === 1 ? 1.0 : 0.0;
        return down;
    };
}

var coolInTimeout = null;
var ignoredEntities = [];

var TELEPORTER_STATES = {
    IDLE: 'idle',
    COOL_IN: 'cool_in',
    TARGETTING: 'targetting',
    TARGETTING_INVALID: 'targetting_invalid',
};

var TARGET = {
    NONE: 'none',            // Not currently targetting anything
    INVISIBLE: 'invisible',  // The current target is an invvsible surface
    INVALID: 'invalid',      // The current target is invalid (wall, ceiling, etc.)
    SURFACE: 'surface',      // The current target is a valid surface
    SEAT: 'seat',            // The current target is a seat
};

function Teleporter() {
    var _this = this;
    this.active = false;
    this.state = TELEPORTER_STATES.IDLE;
    this.currentTarget = TARGET.INVALID;

    this.teleportRayLeftVisible = LaserPointers.createLaserPointer({
        joint: "LeftHand",
        filter: RayPick.PICK_ENTITIES,
        faceAvatar: true,
        centerEndY: false,
        renderStates: teleportRenderStates,
        defaultRenderStates: teleportDefaultRenderStates
    });
    this.teleportRayLeftInvisible = LaserPointers.createLaserPointer({
        joint: "LeftHand",
        filter: RayPick.PICK_ENTITIES | RayPick.PICK_INCLUDE_INVISIBLE,
        faceAvatar: true,
        centerEndY: false,
        renderStates: teleportRenderStates
    });
    this.teleportRayRightVisible = LaserPointers.createLaserPointer({
        joint: "RightHand",
        filter: RayPick.PICK_ENTITIES,
        faceAvatar: true,
        centerEndY: false,
        renderStates: teleportRenderStates,
        defaultRenderStates: teleportDefaultRenderStates
    });
    this.teleportRayRightInvisible = LaserPointers.createLaserPointer({
        joint: "RightHand",
        filter: RayPick.PICK_ENTITIES | RayPick.PICK_INCLUDE_INVISIBLE,
        faceAvatar: true,
        centerEndY: false,
        renderStates: teleportRenderStates
    });

    this.teleportRayHeadVisible = LaserPointers.createLaserPointer({
        joint: "Avatar",
        filter: RayPick.PICK_ENTITIES,
        faceAvatar: true,
        centerEndY: false,
        renderStates: teleportRenderStates,
        defaultRenderStates: teleportDefaultRenderStates
    });
    this.teleportRayHeadInvisible = LaserPointers.createLaserPointer({
        joint: "Avatar",
        filter: RayPick.PICK_ENTITIES | RayPick.PICK_INCLUDE_INVISIBLE,
        faceAvatar: true,
        centerEndY: false,
        renderStates: teleportRenderStates
    });

    this.updateConnected = null;
    this.activeHand = null;

    this.teleporterMappingInternalName = 'Hifi-Teleporter-Internal-Dev-' + Math.random();
    this.teleportMappingInternal = Controller.newMapping(this.teleporterMappingInternalName);

    this.enableMappings = function() {
        Controller.enableMapping(this.teleporterMappingInternalName);
    };

    this.disableMappings = function() {
        Controller.disableMapping(teleporter.teleporterMappingInternalName);
    };

    this.cleanup = function() {
        this.disableMappings();

        LaserPointers.removeLaserPointer(this.teleportRayLeftVisible);
        LaserPointers.removeLaserPointer(this.teleportRayLeftInvisible);
        LaserPointers.removeLaserPointer(this.teleportRayRightVisible);
        LaserPointers.removeLaserPointer(this.teleportRayRightInvisible);
        LaserPointers.removeLaserPointer(this.teleportRayHeadVisible);
        LaserPointers.removeLaserPointer(this.teleportRayHeadInvisible);

        if (this.updateConnected === true) {
            Script.update.disconnect(this, this.update);
        }
    };

    this.enterTeleportMode = function(hand) {
        if (inTeleportMode === true) {
            return;
        }
        if (isDisabled === 'both' || isDisabled === hand) {
            return;
        }

        inTeleportMode = true;

        if (coolInTimeout !== null) {
            Script.clearTimeout(coolInTimeout);
        }

        this.state = TELEPORTER_STATES.COOL_IN;
        coolInTimeout = Script.setTimeout(function() {
            if (_this.state === TELEPORTER_STATES.COOL_IN) {
                _this.state = TELEPORTER_STATES.TARGETTING;
            }
        }, COOL_IN_DURATION);

        this.activeHand = hand;
        this.enableMappings();
        Script.update.connect(this, this.update);
        this.updateConnected = true;
    };

    this.exitTeleportMode = function(value) {
        if (this.updateConnected === true) {
            Script.update.disconnect(this, this.update);
        }

        this.disableMappings();
        LaserPointers.disableLaserPointer(this.teleportRayLeftVisible);
        LaserPointers.disableLaserPointer(this.teleportRayLeftInvisible);
        LaserPointers.disableLaserPointer(this.teleportRayRightVisible);
        LaserPointers.disableLaserPointer(this.teleportRayRightInvisible);
        LaserPointers.disableLaserPointer(this.teleportRayHeadVisible);
        LaserPointers.disableLaserPointer(this.teleportRayHeadInvisible);

        this.updateConnected = null;
        this.state = TELEPORTER_STATES.IDLE;
        inTeleportMode = false;
    };

    this.update = function() {
        if (_this.state === TELEPORTER_STATES.IDLE) {
            return;
        }

        // Get current hand pose information to see if the pose is valid
        var pose = Controller.getPoseValue(handInfo[_this.activeHand].controllerInput);
        var mode = pose.valid ? _this.activeHand : 'head';
        if (!pose.valid) {
            if (mode === 'right') {
                LaserPointers.disableLaserPointer(_this.teleportRayRightVisible);
                LaserPointers.disableLaserPointer(_this.teleportRayRightInvisible);
            } else {
                LaserPointers.disableLaserPointer(_this.teleportRayLeftVisible);
                LaserPointers.disableLaserPointer(_this.teleportRayLeftInvisible);
            }
            LaserPointers.enableLaserPointer(_this.teleportRayHeadVisible);
            LaserPointers.enableLaserPointer(_this.teleportRayHeadInvisible);
        } else {
            if (mode === 'right') {
                LaserPointers.enableLaserPointer(_this.teleportRayRightVisible);
                LaserPointers.enableLaserPointer(_this.teleportRayRightInvisible);
            } else {
                LaserPointers.enableLaserPointer(_this.teleportRayLeftVisible);
                LaserPointers.enableLaserPointer(_this.teleportRayLeftInvisible);
            }
            LaserPointers.disableLaserPointer(_this.teleportRayHeadVisible);
            LaserPointers.disableLaserPointer(_this.teleportRayHeadInvisible);
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
        if (mode === 'right') {
            result = LaserPointers.getPrevRayPickResult(_this.teleportRayRightInvisible);
        } else if (mode === 'left') {
            result = LaserPointers.getPrevRayPickResult(_this.teleportRayLeftInvisible);
        } else {
            result = LaserPointers.getPrevRayPickResult(_this.teleportRayHeadInvisible);
        }

        var teleportLocationType = getTeleportTargetType(result);
        if (teleportLocationType === TARGET.INVISIBLE) {
            if (mode === 'right') {
                result = LaserPointers.getPrevRayPickResult(_this.teleportRayRightVisible);
            } else if (mode === 'left') {
                result = LaserPointers.getPrevRayPickResult(_this.teleportRayLeftVisible);
            } else {
                result = LaserPointers.getPrevRayPickResult(_this.teleportRayHeadVisible);
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


        if (((_this.activeHand === 'left' ? leftPad : rightPad).buttonValue === 0) && inTeleportMode === true) {
            // remember the state before we exit teleport mode and set it back to IDLE
            var previousState = this.state;
            this.exitTeleportMode();

            if (teleportLocationType === TARGET.NONE || teleportLocationType === TARGET.INVALID || previousState === TELEPORTER_STATES.COOL_IN) {
                // Do nothing
            } else if (teleportLocationType === TARGET.SEAT) {
                Entities.callEntityMethod(result.objectID, 'sit');
            } else if (teleportLocationType === TARGET.SURFACE) {
                var offset = getAvatarFootOffset();
                result.intersection.y += offset;
                MyAvatar.goToLocation(result.intersection, false, {x: 0, y: 0, z: 0, w: 1}, false);
                HMD.centerUI();
                MyAvatar.centerBody();
            }
        }
    };

    this.setTeleportState = function(mode, visibleState, invisibleState) {
        if (mode === 'right') {
            LaserPointers.setRenderState(_this.teleportRayRightVisible, visibleState);
            LaserPointers.setRenderState(_this.teleportRayRightInvisible, invisibleState);
        } else if (mode === 'left') {
            LaserPointers.setRenderState(_this.teleportRayLeftVisible, visibleState);
            LaserPointers.setRenderState(_this.teleportRayLeftInvisible, invisibleState);
        } else {
            LaserPointers.setRenderState(_this.teleportRayHeadVisible, visibleState);
            LaserPointers.setRenderState(_this.teleportRayHeadInvisible, invisibleState);
        }
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
        if (footJointIndex != -1) {
            break;
        }
    }
    if (footJointIndex != -1) {
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

var leftPad = new ThumbPad('left');
var rightPad = new ThumbPad('right');
var leftTrigger = new Trigger('left');
var rightTrigger = new Trigger('right');

var mappingName, teleportMapping;

var TELEPORT_DELAY = 0;

function isMoving() {
    var LY = Controller.getValue(Controller.Standard.LY);
    var LX = Controller.getValue(Controller.Standard.LX);
    if (LY !== 0 || LX !== 0) {
        return true;
    } else {
        return false;
    }
}

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
    if (result.type == RayPick.INTERSECTED_NONE) {
        return TARGET.NONE;
    }

    var props = Entities.getEntityProperties(result.objectID, ['userData', 'visible']);
    var data = parseJSON(props.userData);
    if (data !== undefined && data.seat !== undefined) {
        var avatarUuid = Uuid.fromString(data.seat.user);
        if (Uuid.isNull(avatarUuid) || !AvatarList.getAvatar(avatarUuid)) {
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
            Vec3.distance(MyAvatar.position, result.intersection) <= TELEPORT_CANCEL_RANGE) {
        return TARGET.INVALID;
    } else {
        return TARGET.SURFACE;
    }
}

function registerMappings() {
    mappingName = 'Hifi-Teleporter-Dev-' + Math.random();
    teleportMapping = Controller.newMapping(mappingName);
    teleportMapping.from(Controller.Standard.RT).peek().to(rightTrigger.buttonPress);
    teleportMapping.from(Controller.Standard.LT).peek().to(leftTrigger.buttonPress);

    teleportMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(rightPad.buttonPress);
    teleportMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(leftPad.buttonPress);

    teleportMapping.from(Controller.Standard.LeftPrimaryThumb)
        .to(function(value) {
            if (isDisabled === 'left' || isDisabled === 'both') {
                return;
            }
            if (leftTrigger.down()) {
                return;
            }
            if (isMoving() === true) {
                return;
            }
            teleporter.enterTeleportMode('left');
            return;
        });
    teleportMapping.from(Controller.Standard.RightPrimaryThumb)
        .to(function(value) {
            if (isDisabled === 'right' || isDisabled === 'both') {
                return;
            }
            if (rightTrigger.down()) {
                return;
            }
            if (isMoving() === true) {
                return;
            }

            teleporter.enterTeleportMode('right');
            return;
        });
}

registerMappings();

var teleporter = new Teleporter();

Controller.enableMapping(mappingName);

function cleanup() {
    teleportMapping.disable();
    teleporter.cleanup();
}
Script.scriptEnding.connect(cleanup);

var setIgnoreEntities = function() {
    LaserPointers.setIgnoreEntities(teleporter.teleportRayRightVisible, ignoredEntities);
    LaserPointers.setIgnoreEntities(teleporter.teleportRayRightInvisible, ignoredEntities);
    LaserPointers.setIgnoreEntities(teleporter.teleportRayLeftVisible, ignoredEntities);
    LaserPointers.setIgnoreEntities(teleporter.teleportRayLeftInvisible, ignoredEntities);
    LaserPointers.setIgnoreEntities(teleporter.teleportRayHeadVisible, ignoredEntities);
    LaserPointers.setIgnoreEntities(teleporter.teleportRayHeadInvisible, ignoredEntities);
}

var isDisabled = false;
var handleTeleportMessages = function(channel, message, sender) {
    if (sender === MyAvatar.sessionUUID) {
        if (channel === 'Hifi-Teleport-Disabler') {
            if (message === 'both') {
                isDisabled = 'both';
            }
            if (message === 'left') {
                isDisabled = 'left';
            }
            if (message === 'right') {
                isDisabled = 'right';
            }
            if (message === 'none') {
                isDisabled = false;
            }
        } else if (channel === 'Hifi-Teleport-Ignore-Add' && !Uuid.isNull(message) && ignoredEntities.indexOf(message) === -1) {
            ignoredEntities.push(message);
            setIgnoreEntities();
        } else if (channel === 'Hifi-Teleport-Ignore-Remove' && !Uuid.isNull(message)) {
            var removeIndex = ignoredEntities.indexOf(message);
            if (removeIndex > -1) {
                ignoredEntities.splice(removeIndex, 1);
                setIgnoreEntities();
            }
        }
    }
};

Messages.subscribe('Hifi-Teleport-Disabler');
Messages.subscribe('Hifi-Teleport-Ignore-Add');
Messages.subscribe('Hifi-Teleport-Ignore-Remove');
Messages.messageReceived.connect(handleTeleportMessages);

}()); // END LOCAL_SCOPE
