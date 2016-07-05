// by james b. pollack @imgntn on 7/2/2016

//v1
//check if trigger is down xxx
//if trigger is down, check if thumb is down xxx
//if both are down, enter 'teleport mode' xxx
//aim controller to change landing spot xxx
//visualize aim + spot (line + circle) xxx
//release trigger to teleport then exit teleport mode xxx
//if thumb is release, exit teleport mode xxx


//v2
// try just thumb to teleport xxx

//v2
//fade in/out 
//stretchy beam instead of GL line

//v3
//

//try moving to final destination in 4 steps: 50% 75% 90% 100% (arrival)


//terminate the line when there is an intersection (moving away from lines so...)
//when there's not an intersection, set a fixed distance?  (no)

//v2: show room boundaries when choosing a place to teleport
//v2: smooth fade screen in/out?
//v2: haptic feedback

var inTeleportMode = false;

var currentFadeSphereOpacity = 1;
var fadeSphereInterval = null;
//milliseconds between fading one-tenth -- so this is a half second fade total
var FADE_IN_INTERVAL = 50;
var FADE_OUT_INTERVAL = 50;

var BEAM_MODEL_URL = "http://hifi-content.s3.amazonaws.com/james/teleporter/teleportBeam.fbx";
var STRETCHY_BEAM_DIMENSIONS_X = 0.07;
var STRETCHY_BEAM_DIMENSIONS_Y = 0.07;

var TARGET_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/james/teleporter/Tele-destiny.fbx';

var TARGET_MODEL_DIMENSIONS = {
    x: 1.15,
    y: 0.5,
    z: 1.15
};

function ThumbPad(hand) {
    this.hand = hand;
    var _this = this;

    this.buttonPress = function(value) {
        print('jbp pad press: ' + value + " on: " + _this.hand)
        _this.buttonValue = value;
    };

    this.down = function() {
        return _this.buttonValue === 1 ? 1.0 : 0.0;
    };

}

function Trigger(hand) {
    this.hand = hand;
    var _this = this;

    this.buttonPress = function(value) {
        _this.buttonValue = value;

    };

    this.down = function() {
        return _this.buttonValue === 1 ? 1.0 : 0.0;
    };
}

function Teleporter() {
    var _this = this;
    this.targetProps = null;
    this.rightOverlayLine = null;
    this.leftOverlayLine = null;
    this.stretchyBeam = null;
    this.initialize = function() {
        print('jbp initialize')
        this.createMappings();
        this.disableGrab();

        var cameraEuler = Quat.safeEulerAngles(Camera.orientation);
        var towardsMe = Quat.angleAxis(cameraEuler.y + 180, {
            x: 0,
            y: 1,
            z: 0
        });

        var targetOverlayProps = {
            url: TARGET_MODEL_URL,
            position: MyAvatar.position,
            rotation: towardsMe,
            dimensions: TARGET_MODEL_DIMENSIONS
        };

        _this.targetOverlay = Overlays.addOverlay("model", targetOverlayProps);
    };


    this.createMappings = function() {
        print('jbp create mappings internal');
        // peek at the trigger and thumbs to store their values
        teleporter.telporterMappingInternalName = 'Hifi-Teleporter-Internal-Dev-' + Math.random();
        teleporter.teleportMappingInternal = Controller.newMapping(teleporter.telporterMappingInternalName);

        Controller.enableMapping(teleporter.telporterMappingInternalName);
    };

    this.disableMappings = function() {
        print('jbp disable mappings internal')
        Controller.disableMapping(teleporter.telporterMappingInternalName);
    };

    this.enterTeleportMode = function(hand) {
        if (inTeleportMode === true) {
            return;
        };

        print('jbp hand on entering teleport mode: ' + hand);
        inTeleportMode = true;
        this.teleportHand = hand;
        this.initialize();
        this.updateConnected = true;
        Script.update.connect(this.update);

    };

    this.drawStretchyBeamWithoutIntersection = function() {

    };

    this.findMidpoint = function(handPosition, intersection) {
        var xy = Vec3.sum(handPosition, intersection.intersection);
        var midpoint = Vec3.multiply(0.5, xy);
        print('midpoint point is ' + JSON.stringify(midpoint));
        return midpoint
    };


    this.createStretchyBeam = function(handPosition, intersection, rotation) {

        var beamProps = {
            url: BEAM_MODEL_URL,
            position: _this.findMidpoint(handPosition, intersection),
            dimensions: {
                x: STRETCHY_BEAM_DIMENSIONS_X,
                y: STRETCHY_BEAM_DIMENSIONS_Y,
                z: 0.1
            },
            ignoreRayIntersection: true,
        };

        _this.stretchyBeam = Overlays.addOverlay("model", beamProps);
        Script.update.connect(_this.updateStretchyBeam);
    };


    this.updateStretchyBeam = function(handPosition, intersection, rotation) {
        var dimensions = {
            x: STRETCHY_BEAM_DIMENSIONS_X,
            y: STRETCHY_BEAM_DIMENSIONS_Y,
            z: Vec3.distance(handPosition, intersection.intersection) / 2
        };

        var position = _this.findMidpoint(handPosition, intersection);

        Overlays.editOverlay(_this.stretchyBeam, {
            dimensions: dimensions,
            position: position,

        })

    };

    this.deleteStretchyBeam = function() {
        Overlays.deleteOverlay(_this.stretchyBeam);
        Script.update.disconnect(_this.updateStretchyBeam);
    };


    this.createFadeSphere = function(avatarHead) {
        var sphereProps = {
            position: avatarHead,
            size: 0.25,
            color: {
                red: 0,
                green: 0,
                blue: 0,
            },
            alpha: 1,
            solid: true,
            visible: true,
            ignoreRayIntersection: true,
            drawInFront: true
        };

        currentFadeSphereOpacity = 1;

        _this.fadeSphere = Overlays.addOverlay("sphere", sphereProps);

        Script.update.connect(_this.updateFadeSphere);
    };

    this.fadeSphereOut = function() {

        fadeSphereInterval = Script.setInterval(function() {
            if (currentFadeSphereOpacity === 0) {
                Script.clearInterval(fadeSphereInterval);
                _this.deleteFadeSphere();
                fadeSphereInterval = null;
                return;
            }
            if (currentFadeSphereOpacity > 0) {
                currentFadeSphereOpacity -= 0.1;
            }
            Overlays.editOverlay(_this.fadeSphere, {
                alpha: currentFadeSphereOpacity
            })

        }, FADE_OUT_INTERVAL);
    };

    this.fadeSphereIn = function() {
        fadeSphereInterval = Script.setInterval(function() {
            if (currentFadeSphereOpacity === 1) {
                Script.clearInterval(fadeSphereInterval);
                _this.deleteFadeSphere();
                fadeSphereInterval = null;
                return;
            }
            if (currentFadeSphereOpacity < 1) {
                currentFadeSphereOpacity += 0.1;
            }
            Overlays.editOverlay(_this.fadeSphere, {
                alpha: currentFadeSphereOpacity
            })

        }, FADE_IN_INTERVAL);
    };

    this.updateFadeSphere = function() {
        var headPosition = MyAvatar.getHeadPosition();
        Overlays.editOverlay(_this.fadeSphere, {
            position: headPosition
        })
    };

    this.deleteFadeSphere = function() {
        Script.update.disconnect(_this.updateFadeSphere);
        Overlays.deleteOverlay(_this.fadeSphere);
    };

    this.exitTeleportMode = function(value) {
        print('jbp value on exit: ' + value);
        Script.update.disconnect(this.update);
        this.disableMappings();
        this.rightOverlayOff();
        this.leftOverlayOff();
        Overlays.deleteOverlay(_this.targetOverlay);
        this.enableGrab();
        this.updateConnected = false;
        Script.setTimeout(function() {
            inTeleportMode = false;
        }, 100);
    };


    this.update = function() {

        if (teleporter.teleportHand === 'left') {
            teleporter.leftRay();
            if (leftPad.buttonValue === 0) {
                _this.teleport();
                return;
            }

        } else {
            teleporter.rightRay();
            if (rightPad.buttonValue === 0) {
                _this.teleport();
                return;
            }
        }

    };

    this.rightRay = function() {

        var rightPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.RightHand).translation), MyAvatar.position);

        var rightRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.RightHand).rotation)

        var rightPickRay = {
            origin: rightPosition,
            direction: Quat.getUp(rightRotation),
        };

        this.rightPickRay = rightPickRay;

        var location = Vec3.sum(rightPickRay.origin, Vec3.multiply(rightPickRay.direction, 500));
        this.rightLineOn(rightPickRay.origin, location, {
            red: 7,
            green: 36,
            blue: 44
        });
        var rightIntersection = Entities.findRayIntersection(teleporter.rightPickRay, true, [], [this.targetEntity]);

        if (rightIntersection.intersects) {
            this.updateTargetOverlay(rightIntersection);
            if (this.stretchyBeam !== null) {
                this.updateStretchyBeam(rightPickRay.origin, rightIntersection, rightPickRay.direction);
            } else {
                this.createStretchyBeam(rightPickRay.origin, rightIntersection, rightPickRay.direction);
            }
        } else {
            this.noIntersection()
            this.noIntersectionStretchyBeam();
        }
    };

    this.leftRay = function() {
        var leftPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.LeftHand).translation), MyAvatar.position);


        var leftRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.LeftHand).rotation)
        var leftPickRay = {
            origin: leftPosition,
            direction: Quat.getUp(leftRotation),
        };

        this.leftPickRay = leftPickRay;

        var location = Vec3.sum(leftPickRay.origin, Vec3.multiply(leftPickRay.direction, 500));

        this.leftLineOn(leftPickRay.origin, location, {
            red: 7,
            green: 36,
            blue: 44
        });

        var leftIntersection = Entities.findRayIntersection(teleporter.leftPickRay, true, [], []);

        if (leftIntersection.intersects) {
            this.updateTargetOverlay(leftIntersection);
            if (this.stretchyBeam !== null) {
                this.updateStretchyBeam(rightPickRay.origin, rightIntersection, rightPickRay.direction);
            } else {
                this.createStretchyBeam(rightPickRay.origin, rightIntersection, rightPickRay.direction);

            }

        } else {
            this.noIntersection();
            this.noIntersectionStretchyBeam();
        }
    };

    this.rightLineOn = function(closePoint, farPoint, color) {
        // draw a line
        if (this.rightOverlayLine === null) {
            var lineProperties = {
                lineWidth: 50,
                start: closePoint,
                end: farPoint,
                color: color,
                ignoreRayIntersection: true, // always ignore this
                visible: true,
                alpha: 1
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
                alpha: 1
            });
        }
    };

    this.leftLineOn = function(closePoint, farPoint, color) {
        // draw a line
        if (this.leftOverlayLine === null) {
            var lineProperties = {
                lineWidth: 50,
                start: closePoint,
                end: farPoint,
                color: color,
                ignoreRayIntersection: true, // always ignore this
                visible: true,
                alpha: 1
            };

            this.leftOverlayLine = Overlays.addOverlay("line3d", lineProperties);

        } else {
            var success = Overlays.editOverlay(this.leftOverlayLine, {
                lineWidth: 50,
                start: closePoint,
                end: farPoint,
                color: color,
                visible: true,
                ignoreRayIntersection: true, // always ignore this
                alpha: 1
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

    this.noIntersection = function() {
        print('no intersection' + teleporter.targetOverlay);
        Overlays.editOverlay(teleporter.targetOverlay, {
            visible: false,
        });
    };


    this.noIntersectionStretchyBeam = function() {
        print('no intersection');

    };


    this.updateTargetOverlay = function(intersection) {
        this.intersection = intersection;
        var position = {
            x: intersection.intersection.x,
            y: intersection.intersection.y + TARGET_MODEL_DIMENSIONS.y,
            z: intersection.intersection.z
        }
        Overlays.editOverlay(this.targetOverlay, {
            position: position,
            visible: true
        });

    };

    this.disableGrab = function() {
        Messages.sendLocalMessage('Hifi-Hand-Disabler', 'both');
    };

    this.enableGrab = function() {
        Messages.sendLocalMessage('Hifi-Hand-Disabler', 'none');
    };

    this.teleport = function(value) {

        print('TELEPORT CALLED');

        var offset = getAvatarFootOffset();

        _this.intersection.intersection.y += offset;
        MyAvatar.position = _this.intersection.intersection;
        this.exitTeleportMode();
    };
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
}



var leftPad = new ThumbPad('left');
var rightPad = new ThumbPad('right');
var leftTrigger = new Trigger('left');
var rightTrigger = new Trigger('right');

//create a controller mapping and make sure to disable it when the script is stopped

var mappingName, teleportMapping;

function registerMappings() {
    mappingName = 'Hifi-Teleporter-Dev-' + Math.random();
    teleportMapping = Controller.newMapping(mappingName);

    teleportMapping.from(Controller.Standard.RightPrimaryThumb).peek().to(rightPad.buttonPress);
    teleportMapping.from(Controller.Standard.LeftPrimaryThumb).peek().to(leftPad.buttonPress);

    teleportMapping.from(leftPad.down).to(function() {
        teleporter.enterTeleportMode('left')
    });
    teleportMapping.from(rightPad.down).to(function() {
        teleporter.enterTeleportMode('right')
    });
}

registerMappings();
var teleporter = new Teleporter();

Controller.enableMapping(mappingName);

Script.scriptEnding.connect(cleanup);

function cleanup() {
    teleportMapping.disable();
    teleporter.disableMappings();
    teleporter.rightOverlayOff();
    teleporter.leftOverlayOff();
    Overlays.deleteOverlay(teleporter.targetOverlay);
    if (teleporter.updateConnected !== null) {
        Script.update.disconnect(teleporter.update);
    }
}