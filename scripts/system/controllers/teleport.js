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
var TARGET_MODEL_DIMENSIONS = {
    x: 1.15,
    y: 0.5,
    z: 1.15
};

var COLORS_TELEPORT_CAN_TELEPORT = {
    red: 97,
    green: 247,
    blue: 255
}

var COLORS_TELEPORT_CANNOT_TELEPORT = {
    red: 0,
    green: 121,
    blue: 141
};

var COLORS_TELEPORT_TOO_CLOSE = {
    red: 255,
    green: 184,
    blue: 73
};

var TELEPORT_CANCEL_RANGE = 1;
var USE_COOL_IN = true;
var COOL_IN_DURATION = 500;

function ThumbPad(hand) {
    this.hand = hand;
    var _thisPad = this;

    this.buttonPress = function(value) {
        _thisPad.buttonValue = value;
        if (value === 0) {
            if (activationTimeout !== null) {
                Script.clearTimeout(activationTimeout);
                activationTimeout = null;
            }

        }
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
        return down
    };
}

var coolInTimeout = null;

function Teleporter() {
    var _this = this;
    this.intersection = null;
    this.rightOverlayLine = null;
    this.leftOverlayLine = null;
    this.targetOverlay = null;
    this.cancelOverlay = null;
    this.updateConnected = null;
    this.smoothArrivalInterval = null;
    this.teleportHand = null;
    this.tooClose = false;
    this.inCoolIn = false;


    this.initialize = function() {
        this.createMappings();
    };

    this.createMappings = function() {
        teleporter.telporterMappingInternalName = 'Hifi-Teleporter-Internal-Dev-' + Math.random();
        teleporter.teleportMappingInternal = Controller.newMapping(teleporter.telporterMappingInternalName);

        Controller.enableMapping(teleporter.telporterMappingInternalName);
    };

    this.disableMappings = function() {
        Controller.disableMapping(teleporter.telporterMappingInternalName);
    };

    this.enterTeleportMode = function(hand) {

        if (inTeleportMode === true) {
            return;
        }
        if (isDisabled === 'both') {
            return;
        }

        inTeleportMode = true;
        this.inCoolIn = true;
        if (coolInTimeout !== null) {
            Script.clearTimeout(coolInTimeout);

        }
        coolInTimeout = Script.setTimeout(function() {
            _this.inCoolIn = false;
        }, COOL_IN_DURATION)

        if (this.smoothArrivalInterval !== null) {
            Script.clearInterval(this.smoothArrivalInterval);
        }
        if (activationTimeout !== null) {
            Script.clearInterval(activationTimeout);
        }

        this.teleportHand = hand;
        this.initialize();
        Script.update.connect(this.update);
        this.updateConnected = true;



    };

    this.createTargetOverlay = function() {

        if (_this.targetOverlay !== null) {
            return;
        }
        var targetOverlayProps = {
            url: TARGET_MODEL_URL,
            dimensions: TARGET_MODEL_DIMENSIONS,
            visible: true
        };

        var cancelOverlayProps = {
            url: TOO_CLOSE_MODEL_URL,
            dimensions: TARGET_MODEL_DIMENSIONS,
            visible: true
        };

        _this.targetOverlay = Overlays.addOverlay("model", targetOverlayProps);

    };

    this.createCancelOverlay = function() {

        if (_this.cancelOverlay !== null) {
            return;
        }

        var cancelOverlayProps = {
            url: TOO_CLOSE_MODEL_URL,
            dimensions: TARGET_MODEL_DIMENSIONS,
            visible: true
        };

        _this.cancelOverlay = Overlays.addOverlay("model", cancelOverlayProps);
    };

    this.deleteCancelOverlay = function() {
        if (this.cancelOverlay === null) {
            return;
        }

        Overlays.deleteOverlay(this.cancelOverlay);
        this.cancelOverlay = null;
    }

    this.deleteTargetOverlay = function() {
        if (this.targetOverlay === null) {
            return;
        }

        Overlays.deleteOverlay(this.targetOverlay);
        this.intersection = null;
        this.targetOverlay = null;
    }

    this.turnOffOverlayBeams = function() {
        this.rightOverlayOff();
        this.leftOverlayOff();
    }

    this.exitTeleportMode = function(value) {
        if (activationTimeout !== null) {
            Script.clearTimeout(activationTimeout);
            activationTimeout = null;
        }
        if (this.updateConnected === true) {
            Script.update.disconnect(this.update);
        }

        this.disableMappings();
        this.turnOffOverlayBeams();

        this.updateConnected = null;
        this.inCoolIn = false;
        inTeleportMode = false;
    };

    this.update = function() {
        if (isDisabled === 'both') {
            return;
        }

        if (teleporter.teleportHand === 'left') {
            if (isDisabled === 'left') {
                return;
            }
            teleporter.leftRay();
            if ((leftPad.buttonValue === 0) && inTeleportMode === true) {
                if (_this.inCoolIn === true) {
                    _this.exitTeleportMode();
                    _this.deleteTargetOverlay();
                    _this.deleteCancelOverlay();
                } else {
                    _this.teleport();
                }
                return;
            }

        } else {
            if (isDisabled === 'right') {
                return;
            }
            teleporter.rightRay();
            if ((rightPad.buttonValue === 0) && inTeleportMode === true) {
                if (_this.inCoolIn === true) {
                    _this.exitTeleportMode();
                    _this.deleteTargetOverlay();
                    _this.deleteCancelOverlay();
                } else {
                    _this.teleport();
                }
                return;
            }
        }

    };

    this.rightRay = function() {
        var pose = Controller.getPoseValue(Controller.Standard.RightHand);
        var rightPosition = pose.valid ? Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position) : MyAvatar.getHeadPosition();
        var rightRotation = pose.valid ? Quat.multiply(MyAvatar.orientation, pose.rotation) :
            Quat.multiply(MyAvatar.headOrientation, Quat.angleAxis(-90, {
                x: 1,
                y: 0,
                z: 0
            }));

        var rightPickRay = {
            origin: rightPosition,
            direction: Quat.getUp(rightRotation),
        };

        this.rightPickRay = rightPickRay;

        var location = Vec3.sum(rightPickRay.origin, Vec3.multiply(rightPickRay.direction, 50));


        var rightIntersection = Entities.findRayIntersection(teleporter.rightPickRay, true, [], [this.targetEntity], true);

        if (rightIntersection.intersects) {
            if (this.tooClose === true) {
                this.deleteTargetOverlay();

                this.rightLineOn(rightPickRay.origin, rightIntersection.intersection, COLORS_TELEPORT_TOO_CLOSE);
                if (this.cancelOverlay !== null) {
                    this.updateCancelOverlay(rightIntersection);
                } else {
                    this.createCancelOverlay();
                }
            } else {
                if (this.inCoolIn === true) {
                    this.deleteTargetOverlay();
                    this.rightLineOn(rightPickRay.origin, rightIntersection.intersection, COLORS_TELEPORT_TOO_CLOSE);
                    if (this.cancelOverlay !== null) {
                        this.updateCancelOverlay(rightIntersection);
                    } else {
                        this.createCancelOverlay();
                    }
                } else {
                    this.deleteCancelOverlay();

                    this.rightLineOn(rightPickRay.origin, rightIntersection.intersection, COLORS_TELEPORT_CAN_TELEPORT);
                    if (this.targetOverlay !== null) {
                        this.updateTargetOverlay(rightIntersection);
                    } else {
                        this.createTargetOverlay();
                    }
                }


            }

        } else {

            this.deleteTargetOverlay();
            this.rightLineOn(rightPickRay.origin, location, COLORS_TELEPORT_CANNOT_TELEPORT);
        }
    }


    this.leftRay = function() {
        var pose = Controller.getPoseValue(Controller.Standard.LeftHand);
        var leftPosition = pose.valid ? Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, pose.translation), MyAvatar.position) : MyAvatar.getHeadPosition();
        var leftRotation = pose.valid ? Quat.multiply(MyAvatar.orientation, pose.rotation) :
            Quat.multiply(MyAvatar.headOrientation, Quat.angleAxis(-90, {
                x: 1,
                y: 0,
                z: 0
            }));

        var leftPickRay = {
            origin: leftPosition,
            direction: Quat.getUp(leftRotation),
        };

        this.leftPickRay = leftPickRay;

        var location = Vec3.sum(MyAvatar.position, Vec3.multiply(leftPickRay.direction, 50));


        var leftIntersection = Entities.findRayIntersection(teleporter.leftPickRay, true, [], [this.targetEntity], true);

        if (leftIntersection.intersects) {

            if (this.tooClose === true) {
                this.deleteTargetOverlay();
                this.leftLineOn(leftPickRay.origin, leftIntersection.intersection, COLORS_TELEPORT_TOO_CLOSE);
                if (this.cancelOverlay !== null) {
                    this.updateCancelOverlay(leftIntersection);
                } else {
                    this.createCancelOverlay();
                }
            } else {
                if (this.inCoolIn === true) {
                    this.deleteTargetOverlay();
                    this.leftLineOn(leftPickRay.origin, leftIntersection.intersection, COLORS_TELEPORT_TOO_CLOSE);
                    if (this.cancelOverlay !== null) {
                        this.updateCancelOverlay(leftIntersection);
                    } else {
                        this.createCancelOverlay();
                    }
                } else {
                    this.deleteCancelOverlay();
                    this.leftLineOn(leftPickRay.origin, leftIntersection.intersection, COLORS_TELEPORT_CAN_TELEPORT);

                    if (this.targetOverlay !== null) {
                        this.updateTargetOverlay(leftIntersection);
                    } else {
                        this.createTargetOverlay();
                    }
                }


            }


        } else {

            this.deleteTargetOverlay();
            this.leftLineOn(leftPickRay.origin, location, COLORS_TELEPORT_CANNOT_TELEPORT);
        }
    };

    this.rightLineOn = function(closePoint, farPoint, color) {
        if (this.rightOverlayLine === null) {
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

            this.rightOverlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            var success = Overlays.editOverlay(this.rightOverlayLine, {
                start: closePoint,
                end: farPoint,
                color: color
            });
        }
    };

    this.leftLineOn = function(closePoint, farPoint, color) {
        if (this.leftOverlayLine === null) {
            var lineProperties = {
                ignoreRayIntersection: true,
                start: closePoint,
                end: farPoint,
                color: color,
                visible: true,
                alpha: 1,
                solid: true,
                glow: 1.0,
                drawInFront: true
            };

            this.leftOverlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            var success = Overlays.editOverlay(this.leftOverlayLine, {
                start: closePoint,
                end: farPoint,
                color: color
            });
        }
    };
    this.rightOverlayOff = function() {
        if (this.rightOverlayLine !== null) {
            Overlays.deleteOverlay(this.rightOverlayLine);
            this.rightOverlayLine = null;
        }
    };

    this.leftOverlayOff = function() {
        if (this.leftOverlayLine !== null) {
            Overlays.deleteOverlay(this.leftOverlayLine);
            this.leftOverlayLine = null;
        }
    };

    this.updateTargetOverlay = function(intersection) {
        _this.intersection = intersection;

        var rotation = Quat.lookAt(intersection.intersection, MyAvatar.position, Vec3.UP);
        var euler = Quat.safeEulerAngles(rotation);
        var position = {
            x: intersection.intersection.x,
            y: intersection.intersection.y + TARGET_MODEL_DIMENSIONS.y / 2,
            z: intersection.intersection.z
        };

        this.tooClose = isTooCloseToTeleport(position);
        var towardUs = Quat.fromPitchYawRollDegrees(0, euler.y, 0);

        Overlays.editOverlay(this.targetOverlay, {
            position: position,
            rotation: towardUs
        });

    };

    this.updateCancelOverlay = function(intersection) {
        _this.intersection = intersection;

        var rotation = Quat.lookAt(intersection.intersection, MyAvatar.position, Vec3.UP);
        var euler = Quat.safeEulerAngles(rotation);
        var position = {
            x: intersection.intersection.x,
            y: intersection.intersection.y + TARGET_MODEL_DIMENSIONS.y / 2,
            z: intersection.intersection.z
        };

        this.tooClose = isTooCloseToTeleport(position);
        var towardUs = Quat.fromPitchYawRollDegrees(0, euler.y, 0);

        Overlays.editOverlay(this.cancelOverlay, {
            position: position,
            rotation: towardUs
        });
    };

    this.triggerHaptics = function() {
        var hand = this.teleportHand === 'left' ? 0 : 1;
        var haptic = Controller.triggerShortHapticPulse(0.2, hand);
    };

    this.teleport = function(value) {

        if (value === undefined) {
            this.exitTeleportMode();
        }

        if (this.intersection !== null) {
            if (this.tooClose === true) {
                this.exitTeleportMode();
                this.deleteCancelOverlay();
                return;
            }
            var offset = getAvatarFootOffset();
            this.intersection.intersection.y += offset;
            this.exitTeleportMode();
            this.smoothArrival();
        }
    };

    this.findMidpoint = function(start, end) {
        var xy = Vec3.sum(start, end);
        var midpoint = Vec3.multiply(0.5, xy);
        return midpoint
    };

    this.getArrivalPoints = function(startPoint, endPoint) {
        var arrivalPoints = [];
        var i;
        var lastPoint;

        for (i = 0; i < NUMBER_OF_STEPS; i++) {
            if (i === 0) {
                lastPoint = startPoint;
            }
            var newPoint = _this.findMidpoint(lastPoint, endPoint);
            lastPoint = newPoint;
            arrivalPoints.push(newPoint);
        }

        arrivalPoints.push(endPoint);

        return arrivalPoints;
    };

    this.smoothArrival = function() {

        _this.arrivalPoints = _this.getArrivalPoints(MyAvatar.position, _this.intersection.intersection);
        _this.smoothArrivalInterval = Script.setInterval(function() {
            if (_this.arrivalPoints.length === 0) {
                Script.clearInterval(_this.smoothArrivalInterval);
                HMD.centerUI();
                return;
            }
            var landingPoint = _this.arrivalPoints.shift();
            MyAvatar.position = landingPoint;

            if (_this.arrivalPoints.length === 1 || _this.arrivalPoints.length === 0) {
                _this.deleteTargetOverlay();
                _this.deleteCancelOverlay();
            }

        }, SMOOTH_ARRIVAL_SPACING);


    }
}

//related to repositioning the avatar after you teleport
function getAvatarFootOffset() {
    var data = getJointData();
    var upperLeg, lowerLeg, foot, toe, toeTop;
    data.forEach(function(d) {

        var jointName = d.joint;
        if (jointName === "RightUpLeg") {
            upperLeg = d.translation.y;
        }
        if (jointName === "RightLeg") {
            lowerLeg = d.translation.y;
        }
        if (jointName === "RightFoot") {
            foot = d.translation.y;
        }
        if (jointName === "RightToeBase") {
            toe = d.translation.y;
        }
        if (jointName === "RightToe_End") {
            toeTop = d.translation.y;
        }
    })

    var offset = upperLeg + lowerLeg + foot + toe + toeTop;
    offset = offset / 100;
    return offset;
};

function getJointData() {
    var allJointData = [];
    var jointNames = MyAvatar.jointNames;
    jointNames.forEach(function(joint, index) {
        var translation = MyAvatar.getJointTranslation(index);
        var rotation = MyAvatar.getJointRotation(index)
        allJointData.push({
            joint: joint,
            index: index,
            translation: translation,
            rotation: rotation
        });
    });

    return allJointData;
};

var leftPad = new ThumbPad('left');
var rightPad = new ThumbPad('right');
var leftTrigger = new Trigger('left');
var rightTrigger = new Trigger('right');

var mappingName, teleportMapping;

var activationTimeout = null;
var TELEPORT_DELAY = 0;

function isMoving() {
    var LY = Controller.getValue(Controller.Standard.LY);
    var LX = Controller.getValue(Controller.Standard.LX);
    if (LY !== 0 || LX !== 0) {
        return true;
    } else {
        return false;
    }
};

function isTooCloseToTeleport(position) {
    return Vec3.distance(MyAvatar.position, position) <= TELEPORT_CANCEL_RANGE;
};

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
            if (activationTimeout !== null) {
                return
            }
            if (leftTrigger.down()) {
                return;
            }
            if (isMoving() === true) {
                return;
            }
            activationTimeout = Script.setTimeout(function() {
                Script.clearTimeout(activationTimeout);
                activationTimeout = null;
                teleporter.enterTeleportMode('left')
            }, TELEPORT_DELAY)
            return;
        });
    teleportMapping.from(Controller.Standard.RightPrimaryThumb)
        .to(function(value) {
            if (isDisabled === 'right' || isDisabled === 'both') {
                return;
            }
            if (activationTimeout !== null) {
                return
            }
            if (rightTrigger.down()) {
                return;
            }
            if (isMoving() === true) {
                return;
            }

            activationTimeout = Script.setTimeout(function() {
                teleporter.enterTeleportMode('right')
                Script.clearTimeout(activationTimeout);
                activationTimeout = null;
            }, TELEPORT_DELAY)
            return;
        });
};

registerMappings();

var teleporter = new Teleporter();

Controller.enableMapping(mappingName);

Script.scriptEnding.connect(cleanup);

function cleanup() {
    teleportMapping.disable();
    teleporter.disableMappings();
    teleporter.deleteTargetOverlay();
    teleporter.deleteCancelOverlay();
    teleporter.turnOffOverlayBeams();
    if (teleporter.updateConnected !== null) {
        Script.update.disconnect(teleporter.update);
    }
}

var isDisabled = false;
var handleHandMessages = function(channel, message, sender) {
    var data;
    if (sender === MyAvatar.sessionUUID) {
        if (channel === 'Hifi-Teleport-Disabler') {
            if (message === 'both') {
                isDisabled = 'both';
            }
            if (message === 'left') {
                isDisabled = 'left';
            }
            if (message === 'right') {
                isDisabled = 'right'
            }
            if (message === 'none') {
                isDisabled = false;
            }

        }
    }
}

Messages.subscribe('Hifi-Teleport-Disabler');
Messages.messageReceived.connect(handleHandMessages);

}()); // END LOCAL_SCOPE
