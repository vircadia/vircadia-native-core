//
//  pistol.js
//  examples
//
//  Created by Eric Levin on 11/12/2015
//  Copyright 2013 High Fidelity, Inc.
//
//  This is an example script that turns the hydra controllers and mouse into a entity gun.
//  It reads the controller, watches for trigger pulls, and adds a force to any entity it hits

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../libraries/utils.js");
Script.include("../libraries/constants.js");

var animVarName = "rightHandPosition";
var handlerId = null;
var rightHandJoint = "RightHand";
var rightHandJointIndex = MyAvatar.getJointIndex(rightHandJoint);
var hipJoint = 'Hips';
var myHipsJointIndex = MyAvatar.getJointIndex(hipJoint);

var kickBackForce = 0.01;

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
var fireSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Guns/GUN-SHOT2.raw");

var LASER_WIDTH = 2;
var GUN_FORCE = 10;
var POSE_CONTROLS = [Controller.Standard.LeftHand, Controller.Standard.RightHand];
var TRIGGER_CONTROLS = [Controller.Standard.LT, Controller.Standard.RT];
var MIN_THROWER_DELAY = 1000;
var MAX_THROWER_DELAY = 1000;
var RELOAD_INTERVAL = 5;
var GUN_MODEL = HIFI_PUBLIC_BUCKET + "cozza13/gun/m1911-handgun+1.fbx?v=4";
var BULLET_VELOCITY = 10.0;
var GUN_OFFSETS = [{
    x: -0.04,
    y: 0.26,
    z: 0.04
}, {
    x: 0.04,
    y: 0.26,
    z: 0.04
}];

var GUN_ORIENTATIONS = [Quat.fromPitchYawRollDegrees(0, 90, 90), Quat.fromPitchYawRollDegrees(0, -90, 270)];

var BARREL_OFFSETS = [{
    x: -0.12,
    y: 0.12,
    z: 0.04
}, {
    x: 0.12,
    y: 0.12,
    z: 0.04
}];


var mapping = Controller.newMapping();
var validPoses = [false, false];
var barrelVectors = [0, 0];
var barrelTips = [0, 0];


var shootAnything = true;


// For transforming between world space and our avatar's model space. 
var avatarToModelTranslation, avatarToWorldTranslation, avatarToWorldRotation, worldToAvatarRotation;
var avatarToModelRotation = Quat.angleAxis(180, {
    x: 0,
    y: 1,
    z: 0
}); // N.B.: Our C++ angleAxis takes radians, while our javascript angleAxis takes degrees!
var modelToAvatarRotation = Quat.inverse(avatarToModelRotation); // Flip 180 gives same result without inverse, but being explicit to track the math.

function updateMyCoordinateSystem() {
    print("SJHSJKHSJKHS")
    avatarToWorldTranslation = MyAvatar.position;
    avatarToWorldRotation = MyAvatar.orientation;
    worldToAvatarRotation = Quat.inverse(avatarToWorldRotation);
    avatarToModelTranslation = MyAvatar.getJointTranslation(myHipsJointIndex); // Should really be done on the bind pose.
}

// Just math. 

function modelToWorld(modelPoint) {
    var avatarPoint = Vec3.subtract(Vec3.multiplyQbyV(modelToAvatarRotation, modelPoint), avatarToModelTranslation);
    return Vec3.sum(Vec3.multiplyQbyV(avatarToWorldRotation, avatarPoint), avatarToWorldTranslation);
}

function worldToModel(worldPoint) {
    var avatarPoint = Vec3.multiplyQbyV(worldToAvatarRotation, Vec3.subtract(worldPoint, avatarToWorldTranslation));
    return Vec3.multiplyQbyV(avatarToModelRotation, Vec3.sum(avatarPoint, avatarToModelTranslation));
}

function update(deltaTime) {
    // FIXME we should also expose MyAvatar.handPoses[2], MyAvatar.tipPoses[2]
    var tipPoses = [MyAvatar.leftHandTipPose, MyAvatar.rightHandTipPose];

    for (var side = 0; side < 2; side++) {
        // First check if the controller is valid
        var controllerPose = Controller.getPoseValue(POSE_CONTROLS[side]);
        validPoses[side] = controllerPose.valid;
        // Need to adjust the laser
        var tipPose = tipPoses[side];
        var handRotation = tipPoses[side].rotation;
        var barrelOffset = Vec3.multiplyQbyV(handRotation, BARREL_OFFSETS[side]);
        barrelTips[side] = Vec3.sum(tipPose.translation, barrelOffset);
        barrelVectors[side] = Vec3.multiplyQbyV(handRotation, {
            x: 0,
            y: 1,
            z: 0
        });

    }
}

var kickbackAmount = -0.5;
var k = 0;
var decaySpeed = .1;
var kickback = function(animationProperties) {
    var currentTargetHandWorldPosition= Vec3.mix(startingTargetHandWorldPosition, finalTargetHandWorldPosition, k);
    k += decaySpeed;
    // print("WORLD POS " + JSON.stringify(startingTargetHandWorldPosition));
    var targetHandModelPosition = worldToModel(currentTargetHandWorldPosition);
    var result = {};
    result[animVarName] = targetHandModelPosition;
    return result;
}


var finalTargetHandWorldPosition, startingTargetHandWorldPosition;

function startKickback() {
    if (!handlerId) {
        updateMyCoordinateSystem();
        finalTargetHandWorldPosition = MyAvatar.getJointPosition(rightHandJointIndex);
        startingTargetHandWorldPosition = Vec3.sum(finalTargetHandWorldPosition, {x: 0, y: 0.5, z: 0});
        handlerId = MyAvatar.addAnimationStateHandler(kickback, [animVarName]);
        Script.setTimeout(function() {
            MyAvatar.removeAnimationStateHandler(handlerId);
            handlerId = null;
            k = 0;
        }, 200);
    }
}


function triggerChanged(side, value) {
    var pressed = (value != 0);
    if (pressed) {
        Audio.playSound(fireSound, {
            position: barrelTips[side],
            volume: 1.0
        });

        var shotDirection = Vec3.normalize(barrelVectors[side]);
        var pickRay = {
            origin: barrelTips[side],
            direction: shotDirection
        };
        createMuzzleFlash(barrelTips[side]);
        startKickback();

        var intersection = Entities.findRayIntersection(pickRay, true);
        if (intersection.intersects) {
            if (intersection.properties.name === "rat") {
                var forceDirection = JSON.stringify({
                    forceDirection: shotDirection
                });
                Entities.callEntityMethod(intersection.entityID, 'onHit', [forceDirection]);
            } else {
                Script.setTimeout(function() {
                    if (shootAnything) {
                        Entities.editEntity(intersection.entityID, {
                            velocity: Vec3.multiply(shotDirection, GUN_FORCE)
                        });
                    }
                    createWallHit(intersection.intersection);
                }, 50);
            }
        }
    }
}



function scriptEnding() {
    mapping.disable();
    MyAvatar.detachOne(GUN_MODEL);
    MyAvatar.detachOne(GUN_MODEL);
    clearPose();
}

MyAvatar.attach(GUN_MODEL, "LeftHand", GUN_OFFSETS[0], GUN_ORIENTATIONS[0], 0.40);
MyAvatar.attach(GUN_MODEL, "RightHand", GUN_OFFSETS[1], GUN_ORIENTATIONS[1], 0.40);

mapping.from(Controller.Standard.LT).hysteresis(0.1, 0.5).to(function(value) {
    triggerChanged(0, value);
});

mapping.from(Controller.Standard.RT).hysteresis(0.1, 0.5).to(function(value) {
    triggerChanged(1, value);
});
mapping.enable();

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);


function createWallHit(position) {
    var flash = Entities.addEntity({
        type: "ParticleEffect",
        position: position,
        lifetime: 4,
        "name": "Flash Emitter",
        "color": {
            red: 228,
            green: 128,
            blue: 12
        },
        "maxParticles": 1000,
        "lifespan": 0.15,
        "emitRate": 1000,
        "emitSpeed": 1,
        "speedSpread": 0,
        "emitOrientation": {
            "x": -0.4,
            "y": 1,
            "z": -0.2,
            "w": 0.7071068286895752
        },
        "emitDimensions": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "polarStart": 0,
        "polarFinish": Math.PI,
        "azimuthStart": -3.1415927410125732,
        "azimuthFinish": 2,
        "emitAcceleration": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "accelerationSpread": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "particleRadius": 0.03,
        "radiusSpread": 0.02,
        "radiusStart": 0.02,
        "radiusFinish": 0.03,
        "colorSpread": {
            red: 100,
            green: 100,
            blue: 20
        },
        "alpha": 1,
        "alphaSpread": 0,
        "alphaStart": 0,
        "alphaFinish": 0,
        "additiveBlending": true,
        "textures": "http://ericrius1.github.io/PartiArt/assets/star.png"
    });

    Script.setTimeout(function() {
        Entities.editEntity(flash, {
            isEmitting: false
        });
    }, 100);

}

function createMuzzleFlash(position) {
    var smoke = Entities.addEntity({
        type: "ParticleEffect",
        position: position,
        lifetime: 1,
        "name": "Smoke Hit Emitter",
        "maxParticles": 1000,
        "lifespan": 4,
        "emitRate": 20,
        emitSpeed: 0,
        "speedSpread": 0,
        "emitDimensions": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "polarStart": 0,
        "polarFinish": 0,
        "azimuthStart": -3.1415927410125732,
        "azimuthFinish": 3.14,
        "emitAcceleration": {
            "x": 0,
            "y": 0.5,
            "z": 0
        },
        "accelerationSpread": {
            "x": .2,
            "y": 0,
            "z": .2
        },
        "radiusSpread": .04,
        "particleRadius": 0.07,
        "radiusStart": 0.07,
        "radiusFinish": 0.07,
        "alpha": 0.7,
        "alphaSpread": 0,
        "alphaStart": 0,
        "alphaFinish": 0,
        "additiveBlending": 0,
        "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png"
    });
    Script.setTimeout(function() {
        Entities.editEntity(smoke, {
            isEmitting: false
        })
    }, 100);

    var flash = Entities.addEntity({
        type: "ParticleEffect",
        position: position,
        lifetime: 4,
        "name": "Wall hit emitter",
        "color": {
            red: 228,
            green: 128,
            blue: 12
        },
        "maxParticles": 1000,
        "lifespan": 0.1,
        "emitRate": 1000,
        "emitSpeed": 0.5,
        "speedSpread": 0,
        "emitOrientation": {
            "x": -0.4,
            "y": 1,
            "z": -0.2,
            "w": 0.7071068286895752
        },
        "emitDimensions": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "polarStart": 0,
        "polarFinish": Math.PI,
        "azimuthStart": -3.1415927410125732,
        "azimuthFinish": 2,
        "emitAcceleration": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "accelerationSpread": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "particleRadius": 0.05,
        "radiusSpread": 0.01,
        "radiusStart": 0.05,
        "radiusFinish": 0.05,
        "colorSpread": {
            red: 100,
            green: 100,
            blue: 20
        },
        "alpha": 1,
        "alphaSpread": 0,
        "alphaStart": 0,
        "alphaFinish": 0,
        "additiveBlending": true,
        "textures": "http://ericrius1.github.io/PartiArt/assets/star.png"
    });

    Script.setTimeout(function() {
        Entities.editEntity(flash, {
            isEmitting: false
        });
    }, 100)


}