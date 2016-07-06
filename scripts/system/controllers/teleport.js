// by james b. pollack @imgntn on 7/2/2016


///BUG: there is currently something going on when you try to adjust the dimensions of a second overlay model, it seems to be off by a factor of 10 or already changed or something weird.  

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

//v3
//fade in/out 
//stretchy beam instead of GL line xxx


//try moving to final destination in 4 steps: 50% 75% 90% 100% (arrival)

//v?: haptic feedback
//v?: show room boundaries when choosing a place to teleport



//swirly thing, not sure we'll get to use it
// var TARGET_MODEL_URL='http://hifi-content.s3.amazonaws.com/alan/dev/Cyclone-4.fbx';

// var TARGET_MODEL_DIMENSIONS = {
//     x: 0.93,
//     y: 0.93,
//     z: 1.22
// };


var inTeleportMode = false;

var currentFadeSphereOpacity = 1;
var fadeSphereInterval = null;
//milliseconds between fading one-tenth -- so this is a half second fade total
var FADE_IN_INTERVAL = 50;
var FADE_OUT_INTERVAL = 50;

var BEAM_MODEL_URL = "http://hifi-content.s3.amazonaws.com/james/teleporter/teleportBeam.fbx";

var STRETCHY_BEAM_DIMENSIONS_X = 0.04;
var STRETCHY_BEAM_DIMENSIONS_Y = 0.04;
var STRETCHY_BEAM_DIMENSIONS_Z_NO_INTESECTION = 20;

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
    this.intersection = null;
    this.targetProps = null;
    this.targetOverlay = null;
    this.stretchyBeam = null;
    this.noIntersectionStretchyBeam = null;
    this.updateConnected = null;

    this.initialize = function() {
        print('jbp initialize')
        this.createMappings();
        this.disableGrab();
    };

    this.createTargetOverlay = function() {
        print('creating target overlay')


        var targetOverlayProps = {
            url: TARGET_MODEL_URL,
            dimensions: TARGET_MODEL_DIMENSIONS,
            visible: true,
        };

        _this.targetOverlay = Overlays.addOverlay("model", targetOverlayProps);
    };

    this.rotateTargetTorwardMe = function() {
        var cameraEuler = Quat.safeEulerAngles(Camera.orientation);
        print('camera euler' + JSON.stringify(cameraEuler))
        var towardsMe = Quat.angleAxis(cameraEuler.y + 180, {
            x: 0,
            y: 1,
            z: 0
        });

        print('towardsMe' + JSON.stringify(towardsMe))
        return towardsMe
    }


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
        }

        print('jbp hand on entering teleport mode: ' + hand);
        inTeleportMode = true;
        this.teleportHand = hand;
        this.initialize();
        this.updateConnected = true;
        Script.update.connect(this.update);

    };

    this.findMidpoint = function(handPosition, intersection) {
        var xy = Vec3.sum(handPosition, intersection.intersection);
        var midpoint = Vec3.multiply(0.5, xy);
        return midpoint
    };

    this.updateOrCreateStretchyBeam = function(handPosition, intersection, rotation, direction, noIntersection) {

        if (_this.stretchyBeam === null) {
            var beamProps = {
                url: BEAM_MODEL_URL,
                position: MyAvatar.position,

            };

            _this.stretchyBeam = Overlays.addOverlay("model", beamProps);
        }

        var dimensions = {
            x: STRETCHY_BEAM_DIMENSIONS_X,
            y: STRETCHY_BEAM_DIMENSIONS_Y,
            z: intersection !== null ? Vec3.distance(handPosition, intersection.intersection) : STRETCHY_BEAM_DIMENSIONS_Z_NO_INTESECTION
        };

        var position;
        if (noIntersection === true) {
            this.deleteTargetOverlay();
            print('no intersection')
            position = Vec3.sum(handPosition, Vec3.multiply(STRETCHY_BEAM_DIMENSIONS_Z_NO_INTESECTION / 2, direction));
        } else {
            print('intersection, find midpoint')
            position = _this.findMidpoint(handPosition, intersection);
        }

        Overlays.editOverlay(_this.stretchyBeam, {
            dimensions: dimensions,
            position: position,
            rotation: Quat.multiply(rotation, Quat.angleAxis(180, {
                x: 0,
                y: 1,
                z: 0
            }))
        })

    };

    this.deleteStretchyBeam = function() {
        if (_this.stretchyBeam !== null) {
            Overlays.deleteOverlay(_this.stretchyBeam);
            _this.stretchyBeam = null;
        }
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

    this.deleteTargetOverlay = function() {
        Overlays.deleteOverlay(this.targetOverlay);
        this.intersection = null;
        this.targetOverlay = null;
    }

    this.exitTeleportMode = function(value) {
        print('jbp value on exit: ' + value);
        Script.update.disconnect(this.update);
        this.updateConnected = null;
        this.disableMappings();
        this.deleteStretchyBeam();
        this.deleteTargetOverlay();
        this.enableGrab();
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

        var rightFinal = Quat.multiply(rightRotation, Quat.angleAxis(90, {
            x: 1,
            y: 0,
            z: 0
        }));

        this.rightPickRay = rightPickRay;

        var location = Vec3.sum(rightPickRay.origin, Vec3.multiply(rightPickRay.direction, 500));

        var rightIntersection = Entities.findRayIntersection(teleporter.rightPickRay, true, [], [this.targetEntity]);

        if (rightIntersection.intersects) {

            if (this.targetOverlay !== null) {
                this.updateTargetOverlay(rightIntersection);
            } else {
                this.createTargetOverlay();
            }
            this.updateOrCreateStretchyBeam(rightPickRay.origin, rightIntersection, rightFinal, rightPickRay.direction, false);
        } else {

            this.updateOrCreateStretchyBeam(rightPickRay.origin, null, rightFinal, rightPickRay.direction, true);

        }
    }


    this.leftRay = function() {
        var leftPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.LeftHand).translation), MyAvatar.position);

        var leftRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(Controller.Standard.LeftHand).rotation)

        var leftPickRay = {
            origin: leftPosition,
            direction: Quat.getUp(leftRotation),
        };

        var leftFinal = Quat.multiply(leftRotation, Quat.angleAxis(90, {
            x: 1,
            y: 0,
            z: 0
        }));

        this.leftPickRay = leftPickRay;

        var location = Vec3.sum(leftPickRay.origin, Vec3.multiply(leftPickRay.direction, 500));

        var leftIntersection = Entities.findRayIntersection(teleporter.leftPickRay, true, [], [this.targetEntity]);

        if (leftIntersection.intersects) {
            if (this.targetOverlay !== null) {
                this.updateTargetOverlay(leftIntersection);
            } else {
                this.createTargetOverlay();
            }
            this.updateOrCreateStretchyBeam(leftPickRay.origin, leftIntersection, leftFinal, leftPickRay.direction, false);

        } else {

            this.updateOrCreateStretchyBeam(leftPickRay.origin, null, leftFinal, leftPickRay.direction, true);


        }
    };


    this.updateTargetOverlay = function(intersection) {
        this.intersection = intersection;
        print('intersection is: ' + JSON.stringify(intersection))

        // var rotation = Quat.getUp(intersection.intersection.surfaceNormal);

        var rotation = Quat.lookAt(intersection.intersection,MyAvatar.position,Vec3.UP)
        var euler = Quat.safeEulerAngles(rotation)
        var position = {
            x: intersection.intersection.x,
            y: intersection.intersection.y + TARGET_MODEL_DIMENSIONS.y / 2,
            z: intersection.intersection.z
        }
        Overlays.editOverlay(this.targetOverlay, {
            position: position,
            rotation:Quat.fromPitchYawRollDegrees(0,euler.y,0),
            // rotation: this.rotateTargetTorwardMe()
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
        if (_this.intersection !== null) {
            _this.intersection.intersection.y += offset;
            MyAvatar.position = _this.intersection.intersection;
        }
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
};

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
    teleporter.deleteStretchyBeam();
    teleporter.deleteTargetOverlay();
    if (teleporter.updateConnected !== null) {
        Script.update.disconnect(teleporter.update);
    }
}