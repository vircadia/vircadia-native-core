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

var COLORS_TELEPORT_CANNOT_TELEPORT = {
    red: 0,
    green: 121,
    blue: 141
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

    this.overlayLines = {
        left: null,
        right: null,
    };
    this.updateConnected = null;
    this.activeHand = null;

    this.teleporterMappingInternalName = 'Hifi-Teleporter-Internal-Dev-' + Math.random();
    this.teleportMappingInternal = Controller.newMapping(this.teleporterMappingInternalName);

    // Setup overlays
    this.cancelOverlay = Overlays.addOverlay("model", {
        url: TOO_CLOSE_MODEL_URL,
        dimensions: TARGET_MODEL_DIMENSIONS,
        visible: false
    });
    this.targetOverlay = Overlays.addOverlay("model", {
        url: TARGET_MODEL_URL,
        dimensions: TARGET_MODEL_DIMENSIONS,
        visible: false
    });
    this.seatOverlay = Overlays.addOverlay("model", {
        url: SEAT_MODEL_URL,
        dimensions: TARGET_MODEL_DIMENSIONS,
        visible: false
    });

    this.enableMappings = function() {
        Controller.enableMapping(this.teleporterMappingInternalName);
    };

    this.disableMappings = function() {
        Controller.disableMapping(teleporter.teleporterMappingInternalName);
    };

    this.cleanup = function() {
        this.disableMappings();

        Overlays.deleteOverlay(this.targetOverlay);
        this.targetOverlay = null;

        Overlays.deleteOverlay(this.cancelOverlay);
        this.cancelOverlay = null;

        Overlays.deleteOverlay(this.seatOverlay);
        this.seatOverlay = null;

        this.deleteOverlayBeams();
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
        this.deleteOverlayBeams();
        this.hideTargetOverlay();
        this.hideCancelOverlay();

        this.updateConnected = null;
        this.state = TELEPORTER_STATES.IDLE;
        inTeleportMode = false;
    };

    this.deleteOverlayBeams = function() {
        for (var key in this.overlayLines) {
            if (this.overlayLines[key] !== null) {
                Overlays.deleteOverlay(this.overlayLines[key]);
                this.overlayLines[key] = null;
            }
        }
    };

    this.update = function() {
        if (_this.state === TELEPORTER_STATES.IDLE) {
            return;
        }

        // Get current hand pose information so that we can get the direction of the teleport beam
        var pose = Controller.getPoseValue(handInfo[_this.activeHand].controllerInput);
        var handPosition = pose.valid ? Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position) : MyAvatar.getHeadPosition();
        var handRotation = pose.valid ? Quat.multiply(MyAvatar.orientation, pose.rotation) :
            Quat.multiply(MyAvatar.headOrientation, Quat.angleAxis(-90, {
                x: 1,
                y: 0,
                z: 0
            }));

        var pickRay = {
            origin: handPosition,
            direction: Quat.getUp(handRotation),
        };

        // We do up to 2 ray picks to find a teleport location.
        // There are 2 types of teleport locations we are interested in:
        //   1. A visible floor. This can be any entity surface that points within some degree of "up"
        //   2. A seat. The seat can be visible or invisible.
        //
        //  * In the first pass we pick against visible and invisible entities so that we can find invisible seats.
        //    We might hit an invisible entity that is not a seat, so we need to do a second pass.
        //  * In the second pass we pick against visible entities only.
        //
        var intersection = Entities.findRayIntersection(pickRay, true, [], [this.targetEntity].concat(ignoredEntities), false, true);

        var teleportLocationType = getTeleportTargetType(intersection);
        if (teleportLocationType === TARGET.INVISIBLE) {
            intersection = Entities.findRayIntersection(pickRay, true, [], [this.targetEntity].concat(ignoredEntities), true, true);
            teleportLocationType = getTeleportTargetType(intersection);
        }

        if (teleportLocationType === TARGET.NONE) {
            this.hideTargetOverlay();
            this.hideCancelOverlay();
            this.hideSeatOverlay();

            var farPosition = Vec3.sum(pickRay.origin, Vec3.multiply(pickRay.direction, 50));
            this.updateLineOverlay(_this.activeHand, pickRay.origin, farPosition, COLORS_TELEPORT_CANNOT_TELEPORT);
        } else if (teleportLocationType === TARGET.INVALID || teleportLocationType === TARGET.INVISIBLE) {
            this.hideTargetOverlay();
            this.hideSeatOverlay();

            this.updateLineOverlay(_this.activeHand, pickRay.origin, intersection.intersection, COLORS_TELEPORT_CANCEL);
            this.updateDestinationOverlay(this.cancelOverlay, intersection);
        } else if (teleportLocationType === TARGET.SURFACE) {
            if (this.state === TELEPORTER_STATES.COOL_IN) {
                this.hideTargetOverlay();
                this.hideSeatOverlay();

                this.updateLineOverlay(_this.activeHand, pickRay.origin, intersection.intersection, COLORS_TELEPORT_CANCEL);
                this.updateDestinationOverlay(this.cancelOverlay, intersection);
            } else {
                this.hideCancelOverlay();
                this.hideSeatOverlay();

                this.updateLineOverlay(_this.activeHand, pickRay.origin, intersection.intersection,
                                       COLORS_TELEPORT_CAN_TELEPORT);
                this.updateDestinationOverlay(this.targetOverlay, intersection);
            }
        } else if (teleportLocationType === TARGET.SEAT) {
            this.hideCancelOverlay();
            this.hideTargetOverlay();

            this.updateLineOverlay(_this.activeHand, pickRay.origin, intersection.intersection, COLORS_TELEPORT_SEAT);
            this.updateDestinationOverlay(this.seatOverlay, intersection);
        }


        if (((_this.activeHand === 'left' ? leftPad : rightPad).buttonValue === 0) && inTeleportMode === true) {
            // remember the state before we exit teleport mode and set it back to IDLE
            var previousState = this.state;
            this.exitTeleportMode();
            this.hideCancelOverlay();
            this.hideTargetOverlay();
            this.hideSeatOverlay();

            if (teleportLocationType === TARGET.NONE || teleportLocationType === TARGET.INVALID || previousState === TELEPORTER_STATES.COOL_IN) {
                // Do nothing
            } else if (teleportLocationType === TARGET.SEAT) {
                Entities.callEntityMethod(intersection.entityID, 'sit');
            } else if (teleportLocationType === TARGET.SURFACE) {
                var offset = getAvatarFootOffset();
                intersection.intersection.y += offset;
                MyAvatar.goToLocation(intersection.intersection, false, {x: 0, y: 0, z: 0, w: 1}, false);
                HMD.centerUI();
                MyAvatar.centerBody();
            }
        }
    };

    this.updateLineOverlay = function(hand, closePoint, farPoint, color) {
        if (this.overlayLines[hand] === null) {
            var lineProperties = {
                start: closePoint,
                end: farPoint,
                color: color,
                ignoreRayIntersection: true,
                visible: true,
                alpha: 1,
                solid: true,
                drawInFront: true,
                glow: 1.0
            };

            this.overlayLines[hand] = Overlays.addOverlay("line3d", lineProperties);

        } else {
            Overlays.editOverlay(this.overlayLines[hand], {
                start: closePoint,
                end: farPoint,
                color: color
            });
        }
    };

    this.hideCancelOverlay = function() {
        Overlays.editOverlay(this.cancelOverlay, { visible: false });
    };

    this.hideTargetOverlay = function() {
        Overlays.editOverlay(this.targetOverlay, { visible: false });
    };

    this.hideSeatOverlay = function() {
        Overlays.editOverlay(this.seatOverlay, { visible: false });
    };

    this.updateDestinationOverlay = function(overlayID, intersection) {
        var rotation = Quat.lookAt(intersection.intersection, MyAvatar.position, Vec3.UP);
        var euler = Quat.safeEulerAngles(rotation);
        var position = {
            x: intersection.intersection.x,
            y: intersection.intersection.y + TARGET_MODEL_DIMENSIONS.y / 2,
            z: intersection.intersection.z
        };

        var towardUs = Quat.fromPitchYawRollDegrees(0, euler.y, 0);

        Overlays.editOverlay(overlayID, {
            visible: true,
            position: position,
            rotation: towardUs
        });

    };
}

// related to repositioning the avatar after you teleport
function getAvatarFootOffset() {
    var data = getJointData();
    var upperLeg, lowerLeg, foot, toe, toeTop;
    data.forEach(function(d) {

        var jointName = d.joint;
        if (jointName === "RightUpLeg") {
            upperLeg = d.translation.y;
        } else if (jointName === "RightLeg") {
            lowerLeg = d.translation.y;
        } else if (jointName === "RightFoot") {
            foot = d.translation.y;
        } else if (jointName === "RightToeBase") {
            toe = d.translation.y;
        } else if (jointName === "RightToe_End") {
            toeTop = d.translation.y;
        }
    });

    var offset = upperLeg + lowerLeg + foot + toe + toeTop;
    offset = offset / 100;
    return offset;
}

function getJointData() {
    var allJointData = [];
    var jointNames = MyAvatar.jointNames;
    jointNames.forEach(function(joint, index) {
        var translation = MyAvatar.getJointTranslation(index);
        var rotation = MyAvatar.getJointRotation(index);
        allJointData.push({
            joint: joint,
            index: index,
            translation: translation,
            rotation: rotation
        });
    });

    return allJointData;
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
function getTeleportTargetType(intersection) {
    if (!intersection.intersects) {
        return TARGET.NONE;
    }

    var props = Entities.getEntityProperties(intersection.entityID, ['userData', 'visible']);
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

    var surfaceNormal = intersection.surfaceNormal;
    var adj = Math.sqrt(surfaceNormal.x * surfaceNormal.x + surfaceNormal.z * surfaceNormal.z);
    var angleUp = Math.atan2(surfaceNormal.y, adj) * (180 / Math.PI);

    if (angleUp < (90 - MAX_ANGLE_FROM_UP_TO_TELEPORT) ||
            angleUp > (90 + MAX_ANGLE_FROM_UP_TO_TELEPORT) ||
            Vec3.distance(MyAvatar.position, intersection.intersection) <= TELEPORT_CANCEL_RANGE) {
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
        } else if (channel === 'Hifi-Teleport-Ignore-Remove' && !Uuid.isNull(message)) {
            var removeIndex = ignoredEntities.indexOf(message);
            if (removeIndex > -1) {
                ignoredEntities.splice(removeIndex, 1);
            }
        }
    }
};

Messages.subscribe('Hifi-Teleport-Disabler');
Messages.subscribe('Hifi-Teleport-Ignore-Add');
Messages.subscribe('Hifi-Teleport-Ignore-Remove');
Messages.messageReceived.connect(handleTeleportMessages);

}()); // END LOCAL_SCOPE
