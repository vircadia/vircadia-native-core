// Created by james b. pollack @imgntn on 7/2/2016
// Copyright 2016 High Fidelity, Inc.
//
//  Creates a beam and target and then teleports you there when you let go of either activation button.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var inTeleportMode = false;

var currentFadeSphereOpacity = 1;
var fadeSphereInterval = null;
var fadeSphereUpdateInterval = null;
//milliseconds between fading one-tenth -- so this is a half second fade total
var USE_FADE_MODE = false;
var USE_FADE_OUT = true;
var FADE_OUT_INTERVAL = 25;

// instant
// var NUMBER_OF_STEPS = 0;
// var SMOOTH_ARRIVAL_SPACING = 0;

// // slow
// var SMOOTH_ARRIVAL_SPACING = 150;
// var NUMBER_OF_STEPS = 2;

// medium-slow
// var SMOOTH_ARRIVAL_SPACING = 100;
// var NUMBER_OF_STEPS = 4;

// medium-fast
var SMOOTH_ARRIVAL_SPACING = 33;
var NUMBER_OF_STEPS = 6;

//fast
// var SMOOTH_ARRIVAL_SPACING = 10;
// var NUMBER_OF_STEPS = 20;


var TARGET_MODEL_URL = Script.resolvePath("../assets/models/teleport.fbx");
var TARGET_MODEL_DIMENSIONS = {
    x: 1.15,
    y: 0.5,
    z: 1.15

};


var COLORS_TELEPORT_CAN_TELEPORT = {
    red: 136,
    green: 255,
    blue: 241
};
var COLORS_TELEPORT_CANNOT_TELEPORT = {
    red: 68,
    green: 183,
    blue: 180
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
        return down
    };
}

function Teleporter() {
    var _this = this;
    this.intersection = null;
    this.rightOverlayLine = null;
    this.leftOverlayLine = null;
    this.targetOverlay = null;
    this.updateConnected = null;
    this.smoothArrivalInterval = null;
    this.fadeSphere = null;
    this.teleportHand = null;

    this.initialize = function() {
        this.createMappings();
        this.disableGrab();
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

        _this.targetOverlay = Overlays.addOverlay("model", targetOverlayProps);
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
        inTeleportMode = true;
        if (this.smoothArrivalInterval !== null) {
            Script.clearInterval(this.smoothArrivalInterval);
        }
        if (fadeSphereInterval !== null) {
            Script.clearInterval(fadeSphereInterval);
        }
        this.teleportHand = hand;
        this.initialize();
        Script.update.connect(this.update);
        this.updateConnected = true;
    };

    this.createFadeSphere = function(avatarHead) {
        var sphereProps = {
            position: avatarHead,
            size: -1,
            color: {
                red: 0,
                green: 0,
                blue: 0,
            },
            alpha: 1,
            solid: true,
            visible: true,
            ignoreRayIntersection: true,
            drawInFront: false
        };

        currentFadeSphereOpacity = 10;

        _this.fadeSphere = Overlays.addOverlay("sphere", sphereProps);
        Script.clearInterval(fadeSphereInterval)
        Script.update.connect(_this.updateFadeSphere);
        if (USE_FADE_OUT === true) {
            this.fadeSphereOut();
        }


    };

    this.fadeSphereOut = function() {

        fadeSphereInterval = Script.setInterval(function() {
            if (currentFadeSphereOpacity <= 0) {
                Script.clearInterval(fadeSphereInterval);
                _this.deleteFadeSphere();
                fadeSphereInterval = null;
                return;
            }
            if (currentFadeSphereOpacity > 0) {
                currentFadeSphereOpacity = currentFadeSphereOpacity - 1;
            }

            Overlays.editOverlay(_this.fadeSphere, {
                alpha: currentFadeSphereOpacity / 10
            })

        }, FADE_OUT_INTERVAL);
    };


    this.updateFadeSphere = function() {
        var headPosition = MyAvatar.getHeadPosition();
        Overlays.editOverlay(_this.fadeSphere, {
            position: headPosition
        })
    };

    this.deleteFadeSphere = function() {
        if (_this.fadeSphere !== null) {
            Script.update.disconnect(_this.updateFadeSphere);
            Overlays.deleteOverlay(_this.fadeSphere);
            _this.fadeSphere = null;
        }

    };

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
        if (this.updateConnected === true) {
            Script.update.disconnect(this.update);
        }
        this.disableMappings();
        this.turnOffOverlayBeams();


        this.updateConnected = null;

        Script.setTimeout(function() {
            inTeleportMode = false;
            _this.enableGrab();
        }, 100);
    };



    this.update = function() {

        if (teleporter.teleportHand === 'left') {
            teleporter.leftRay();

            if ((leftPad.buttonValue === 0 || leftTrigger.buttonValue === 0) && inTeleportMode === true) {
                _this.teleport();
                return;
            }

        } else {
            teleporter.rightRay();

            if ((rightPad.buttonValue === 0 || rightTrigger.buttonValue === 0) && inTeleportMode === true) {
                _this.teleport();
                return;
            }
        }

    };

    this.rightRay = function() {


        var rightPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.RightHand).translation), MyAvatar.position);

        var rightControllerRotation = Controller.getPoseValue(Controller.Standard.RightHand).rotation;

        var rightRotation = Quat.multiply(MyAvatar.orientation, rightControllerRotation)


        var rightFinal = Quat.multiply(rightRotation, Quat.angleAxis(90, {
            x: 1,
            y: 0,
            z: 0
        }));

        var rightPickRay = {
            origin: rightPosition,
            direction: Quat.getUp(rightRotation),
        };

        this.rightPickRay = rightPickRay;

        var location = Vec3.sum(rightPickRay.origin, Vec3.multiply(rightPickRay.direction, 500));


        var rightIntersection = Entities.findRayIntersection(teleporter.rightPickRay, true, [], [this.targetEntity]);

        if (rightIntersection.intersects) {
            this.rightLineOn(rightPickRay.origin, rightIntersection.intersection, COLORS_TELEPORT_CAN_TELEPORT);
            if (this.targetOverlay !== null) {
                this.updateTargetOverlay(rightIntersection);
            } else {
                this.createTargetOverlay();
            }

        } else {

            this.deleteTargetOverlay();
            this.rightLineOn(rightPickRay.origin, location, COLORS_TELEPORT_CANNOT_TELEPORT);
        }
    }


    this.leftRay = function() {
        var leftPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.LeftHand).translation), MyAvatar.position);

        var leftRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.LeftHand).rotation)


        var leftFinal = Quat.multiply(leftRotation, Quat.angleAxis(90, {
            x: 1,
            y: 0,
            z: 0
        }));


        var leftPickRay = {
            origin: leftPosition,
            direction: Quat.getUp(leftRotation),
        };

        this.leftPickRay = leftPickRay;

        var location = Vec3.sum(MyAvatar.position, Vec3.multiply(leftPickRay.direction, 500));


        var leftIntersection = Entities.findRayIntersection(teleporter.leftPickRay, true, [], [this.targetEntity]);

        if (leftIntersection.intersects) {

            this.leftLineOn(leftPickRay.origin, leftIntersection.intersection, COLORS_TELEPORT_CAN_TELEPORT);
            if (this.targetOverlay !== null) {
                this.updateTargetOverlay(leftIntersection);
            } else {
                this.createTargetOverlay();
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
                ignoreRayIntersection: true, // always ignore this
                visible: true,
                alpha: 1,
                solid: true,
                drawInFront: true,
                glow: 1.0
            };

            this.rightOverlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            var success = Overlays.editOverlay(this.rightOverlayLine, {
                lineWidth: 50,
                start: closePoint,
                end: farPoint,
                color: color,
                visible: true,
                ignoreRayIntersection: true, // always ignore this
                alpha: 1,
                glow: 1.0
            });
        }
    };

    this.leftLineOn = function(closePoint, farPoint, color) {
        if (this.leftOverlayLine === null) {
            var lineProperties = {
                ignoreRayIntersection: true, // always ignore this
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
                color: color,
                visible: true,
                alpha: 1,
                solid: true,
                glow: 1.0
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

        var rotation = Quat.lookAt(intersection.intersection, MyAvatar.position, Vec3.UP)
        var euler = Quat.safeEulerAngles(rotation)
        var position = {
            x: intersection.intersection.x,
            y: intersection.intersection.y + TARGET_MODEL_DIMENSIONS.y / 2,
            z: intersection.intersection.z
        }
        Overlays.editOverlay(this.targetOverlay, {
            position: position,
            rotation: Quat.fromPitchYawRollDegrees(0, euler.y, 0),
        });

    };

    this.disableGrab = function() {
        Messages.sendLocalMessage('Hifi-Hand-Disabler', this.teleportHand);
    };

    this.enableGrab = function() {
        Messages.sendLocalMessage('Hifi-Hand-Disabler', 'none');
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
            if (USE_FADE_MODE === true) {
                this.createFadeSphere();
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

        arrivalPoints.push(endPoint)

        return arrivalPoints
    };

    this.smoothArrival = function() {

        _this.arrivalPoints = _this.getArrivalPoints(MyAvatar.position, _this.intersection.intersection);
        _this.smoothArrivalInterval = Script.setInterval(function() {
            if (_this.arrivalPoints.length === 0) {
                Script.clearInterval(_this.smoothArrivalInterval);
                return;
            }

            var landingPoint = _this.arrivalPoints.shift();
            MyAvatar.position = landingPoint;

            if (_this.arrivalPoints.length === 1 || _this.arrivalPoints.length === 0) {
                _this.deleteTargetOverlay();
            }


        }, SMOOTH_ARRIVAL_SPACING)
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
            toeTop = d.translation.y
        }
    })

    var myPosition = MyAvatar.position;
    var offset = upperLeg + lowerLeg + foot + toe + toeTop;
    offset = offset / 100;
    return offset
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

var TELEPORT_DELAY = 100;


function registerMappings() {
    mappingName = 'Hifi-Teleporter-Dev-' + Math.random();
    teleportMapping = Controller.newMapping(mappingName);
    teleportMapping.from(Controller.Standard.RT).peek().to(rightTrigger.buttonPress);
    teleportMapping.from(Controller.Standard.LT).peek().to(leftTrigger.buttonPress);

    teleportMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(rightPad.buttonPress);
    teleportMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(leftPad.buttonPress);

    teleportMapping.from(Controller.Standard.LeftPrimaryThumb).when(leftTrigger.down).to(function(value) {
        teleporter.enterTeleportMode('left')
        return;
    });
    teleportMapping.from(Controller.Standard.RightPrimaryThumb).when(rightTrigger.down).to(function(value) {
        teleporter.enterTeleportMode('right')
        return;
    });
    teleportMapping.from(Controller.Standard.RT).when(Controller.Standard.RightPrimaryThumb).to(function(value) {
        teleporter.enterTeleportMode('right')
        return;
    });
    teleportMapping.from(Controller.Standard.LT).when(Controller.Standard.LeftPrimaryThumb).to(function(value) {
        teleporter.enterTeleportMode('left')
        return;
    });

}

registerMappings();

var teleporter = new Teleporter();

Controller.enableMapping(mappingName);

Script.scriptEnding.connect(cleanup);

function cleanup() {
    teleportMapping.disable();
    teleporter.disableMappings();
    teleporter.deleteTargetOverlay();
    teleporter.turnOffOverlayBeams();
    teleporter.deleteFadeSphere();
    if (teleporter.updateConnected !== null) {
        Script.update.disconnect(teleporter.update);
    }
}