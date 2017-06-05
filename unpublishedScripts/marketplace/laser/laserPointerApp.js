'use strict';

(function () {
    Script.include("/~/system/libraries/controllers.js");

    var APP_NAME = 'LASER',
        APP_ICON = 'https://binaryrelay.com/files/public-docs/hifi/laser/laser.svg',
        APP_ICON_ACTIVE = 'https://binaryrelay.com/files/public-docs/hifi/laser/laser-a.svg';

    var POINT_INDEX_CHANNEL = "Hifi-Point-Index",
        GRAB_DISABLE_CHANNEL = "Hifi-Grab-Disable",
        POINTER_DISABLE_CHANNEL = "Hifi-Pointer-Disable";

    var TRIGGER_PRESSURE = 0.95;

    var tablet = Tablet.getTablet('com.highfidelity.interface.tablet.system');

    var button = tablet.addButton({
        icon: APP_ICON,
        activeIcon: APP_ICON_ACTIVE,
        text: APP_NAME
    });

    var laserEntities = {
        left: {
            beam: null,
            sphere: null
        },
        right: {
            beam: null,
            sphere: null
        }
    };

    var rayExclusionList = [];


    function laser(hand) {

        var PICK_MAX_DISTANCE = 500;

        var isNewEntityNeeded = (laserEntities[hand].beam === null);

        var currentHand = hand === 'right' ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var jointName = hand === 'right' ? 'RightHand' : 'LeftHand'; //'RightHandIndex4' : 'LeftHandIndex4'
        var controllerLocation = getControllerWorldLocation(currentHand, true);

        var worldHandRotation = controllerLocation.orientation;

        var pickRay = {
            origin: MyAvatar.getJointPosition(jointName),
            direction: Quat.getUp(worldHandRotation),
            length: PICK_MAX_DISTANCE
        };


        var ray = Entities.findRayIntersection(pickRay, true, [], rayExclusionList, true);
        var avatarRay = AvatarManager.findRayIntersection(pickRay, true, [], rayExclusionList, true);

        var dist = PICK_MAX_DISTANCE;
        var intersection = null;

        if (avatarRay.intersects) {
            intersection = avatarRay.intersection;
            dist = Vec3.distance(pickRay.origin, avatarRay.intersection);
        } else if (ray.intersects) {
            intersection = ray.intersection;
            dist = Vec3.distance(pickRay.origin, ray.intersection);
        }

        if (!ray.intersects && !avatarRay.intersects && laserEntities[hand].sphere !== null) {
            Entities.editEntity(laserEntities[hand].sphere, {
                visible: false
            });
        } else {
            Entities.editEntity(laserEntities[hand].sphere, {
                visible: true
            });
        }

        var sphereSize = dist * 0.01;

        if (isNewEntityNeeded) {

            var sphere = {
                lifetime: 360,
                type: 'Sphere',
                dimensions: {x: sphereSize, y: sphereSize, z: sphereSize},
                color: {red: 0, green: 255, blue: 0},
                position: intersection,
                collisionless: true

            };

            var beam = {
                lifetime: 360,
                type: 'Line',
                glow: 1.0,
                lineWidth: 5,
                alpha: 0.5,
                ignoreRayIntersection: true,
                drawInFront: true,
                color: {red: 0, green: 255, blue: 0},
                parentID: MyAvatar.sessionUUID,
                parentJointIndex: MyAvatar.getJointIndex(jointName),
                localPosition: {x: 0, y: .2, z: 0},
                localRotation: Quat.normalize({}),
                dimensions: Vec3.multiply(PICK_MAX_DISTANCE * 2, Vec3.ONE),
                linePoints: [Vec3.ZERO, {x: 0, y: dist, z: 0}]
            };


            laserEntities[hand].beam = Entities.addEntity(beam,true);
            rayExclusionList.push(laserEntities[hand].beam);

            if (ray.intersects || avatarRay.intersects) {
                laserEntities[hand].sphere = Entities.addEntity(sphere,true);
                rayExclusionList.push(laserEntities[hand].sphere);
            }

        } else {
            if (ray.intersects || avatarRay.intersects) {

                Entities.editEntity(laserEntities[hand].beam, {
                    parentID: MyAvatar.sessionUUID,
                    parentJointIndex: MyAvatar.getJointIndex(jointName),
                    localPosition: {x: 0, y: .2, z: 0},
                    localRotation: Quat.normalize({}),
                    dimensions: Vec3.multiply(PICK_MAX_DISTANCE * 2, Vec3.ONE),
                    linePoints: [Vec3.ZERO, {x: 0, y: dist, z: 0}]
                });

                Entities.editEntity(laserEntities[hand].sphere, {
                    dimensions: {x: sphereSize, y: sphereSize, z: sphereSize},
                    position: intersection

                });
            } else {
                Entities.editEntity(laserEntities[hand].beam, {
                    parentID: MyAvatar.sessionUUID,
                    parentJointIndex: MyAvatar.getJointIndex(jointName),
                    localPosition: {x: 0, y: .2, z: 0},
                    localRotation: Quat.normalize({}),
                    dimensions: Vec3.multiply(PICK_MAX_DISTANCE * 2, Vec3.ONE),
                    linePoints: [Vec3.ZERO, {x: 0, y: dist, z: 0}]
                });
            }

        }

    }

    function triggerWatcher(deltaTime) {

        var deleteBeamLeft = true,
            deleteBeamRight = true;

        if (Controller.getValue(Controller.Standard.LT) > TRIGGER_PRESSURE) {
            deleteBeamLeft = false;
            laser('left');
        }

        if (Controller.getValue(Controller.Standard.RT) > TRIGGER_PRESSURE) {
            deleteBeamRight = false;
            laser('right');
        }

        if (deleteBeamLeft && laserEntities.left.beam !== null) {
            Entities.deleteEntity(laserEntities.left.beam);
            Entities.deleteEntity(laserEntities.left.sphere);

            laserEntities.left.beam = null;
            laserEntities.left.sphere = null;

        }
        if (deleteBeamRight && laserEntities.right.beam !== null) {
            Entities.deleteEntity(laserEntities.right.beam);
            Entities.deleteEntity(laserEntities.right.sphere);

            laserEntities.right.beam = null;
            laserEntities.right.sphere = null;

        }
        if (deleteBeamRight && deleteBeamLeft) {
            rayExclusionList = [];
        }
    }

    function selectionBeamSwitch(bool) {
        Messages.sendMessage(GRAB_DISABLE_CHANNEL, JSON.stringify({
            holdEnabled: bool,
            nearGrabEnabled: bool,
            farGrabEnabled: bool
        }), true);
        Messages.sendMessage(POINTER_DISABLE_CHANNEL, JSON.stringify({
            pointerEnabled: bool
        }), true);
        Messages.sendMessage(POINT_INDEX_CHANNEL, JSON.stringify({
            pointIndex: !bool
        }), true);
    }

    var _switch = true;

    function buttonSwitch() {
        if (_switch) {
            Script.update.connect(triggerWatcher);
            Messages.subscribe(POINT_INDEX_CHANNEL);
            Messages.subscribe(GRAB_DISABLE_CHANNEL);
            Messages.subscribe(POINTER_DISABLE_CHANNEL);
        } else {
            Script.update.disconnect(triggerWatcher);
            Messages.unsubscribe(POINT_INDEX_CHANNEL);
            Messages.unsubscribe(GRAB_DISABLE_CHANNEL);
            Messages.unsubscribe(POINTER_DISABLE_CHANNEL);
        }
        button.editProperties({isActive: _switch});

        selectionBeamSwitch(!_switch);

        _switch = !_switch;
    }

    button.clicked.connect(buttonSwitch);

    function clean() {
        tablet.removeButton(button);
        Script.update.disconnect(triggerWatcher);

        Messages.unsubscribe(POINT_INDEX_CHANNEL);
        Messages.unsubscribe(GRAB_DISABLE_CHANNEL);
        Messages.unsubscribe(POINTER_DISABLE_CHANNEL);
        rayExclusionList = [];
    }

    Script.scriptEnding.connect(clean);
}());