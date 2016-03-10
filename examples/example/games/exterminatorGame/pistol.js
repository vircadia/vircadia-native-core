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


Script.include("../../../libraries/utils.js");
Script.include("../../../libraries/constants.js");

var GUN_FORCE =20;

Messages.sendMessage('Hifi-Hand-Disabler', "both");

var gameName = "Kill All The Rats!"
// var HOST = "localhost:5000"
var HOST = "desolate-bastion-1742.herokuapp.com";
var socketClient = new WebSocket("ws://" + HOST);
var username = GlobalServices.username;
var currentScore = 0;

function score() {
    currentScore++;
    socketClient.send(JSON.stringify({
        username: username,
        score: currentScore,
        gameName: gameName
    }))
}


HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
var fireSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Guns/GUN-SHOT2.raw");
var LASER_LENGTH = 100;
var LASER_WIDTH = 2;
var POSE_CONTROLS = [Controller.Standard.LeftHand, Controller.Standard.RightHand];
var TRIGGER_CONTROLS = [Controller.Standard.LT, Controller.Standard.RT];
var MIN_THROWER_DELAY = 1000;
var MAX_THROWER_DELAY = 1000;
var RELOAD_INTERVAL = 5;
var GUN_MODEL = HIFI_PUBLIC_BUCKET + "cozza13/gun/m1911-handgun+1.fbx?v=4";
var BULLET_VELOCITY = 10.0;
var GUN_OFFSETS = [{
    x: 0.04,
    y: 0.26,
    z: 0.04
}, {
    x: 0.04,
    y: 0.26,
    z: 0.04
}];

var GUN_ORIENTATIONS = [Quat.fromPitchYawRollDegrees(0, 90, 90), Quat.fromPitchYawRollDegrees(0, -90, 270)];

//x -> y
//y -> z
// z -> x
var BARREL_OFFSETS = [ {
    x: -0.12,
    y: 0.12,
    z: 0.04
}, {
    x: 0.12,
    y: 0.12,
    z: 0.04
} ];



var pointers = [];

pointers.push(Overlays.addOverlay("line3d", {
    start: ZERO_VECTOR,
    end: ZERO_VECTOR,
    color: COLORS.RED,
    alpha: 1,
    visible: true,
    lineWidth: LASER_WIDTH
}));

pointers.push(Overlays.addOverlay("line3d", {
    start: ZERO_VECTOR,
    end: ZERO_VECTOR,
    color: COLORS.RED,
    alpha: 1,
    visible: true,
    lineWidth: LASER_WIDTH
}));

var mapping = Controller.newMapping();
var validPoses = [false, false];
var barrelVectors = [0, 0];
var barrelTips = [0, 0];


// If enabled, anything can be shot, otherwise, an entity needs to have "isShootable" set in its userData
var shootAnything = true;


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

        var laserTip = Vec3.sum(Vec3.multiply(LASER_LENGTH, barrelVectors[side]), barrelTips[side]);
        // Update Lasers
        Overlays.editOverlay(pointers[side], {
            start: barrelTips[side],
            end: laserTip,
            alpha: 1,
        });

    }
}



function displayPointer(side) {
    Overlays.editOverlay(pointers[side], {
        visible: true
    });
}

function hidePointer(side) {
    Overlays.editOverlay(pointers[side], {
        visible: false
    });
}

function fire(side, value) {
    if (value == 0) {
        return;
    }
    Audio.playSound(fireSound, {
        position: barrelTips[side],
        volume: 0.5
    });

    var shotDirection = Vec3.normalize(barrelVectors[side]);
    var pickRay = {
        origin: barrelTips[side],
        direction: shotDirection
    };
    createMuzzleFlash(barrelTips[side]);

    var intersection = Entities.findRayIntersectionBlocking(pickRay, true);
    if (intersection.intersects) {
        Script.setTimeout(function() {
            createEntityHitEffect(intersection.intersection);
            if (shootAnything && intersection.properties.dynamic === 1) {
                // Any dynamic entity can be shot
                Entities.editEntity(intersection.entityID, {
                    velocity: Vec3.multiply(shotDirection, GUN_FORCE)
                });
            }

            if (intersection.properties.name === "rat") {
                score();
                createBloodSplatter(intersection.intersection);
                Entities.deleteEntity(intersection.entityID);

            }
            //Attempt to call entity method's shot method
            var forceDirection = JSON.stringify({
                forceDirection: shotDirection
            });
            Entities.callEntityMethod(intersection.entityID, 'onShot', [forceDirection]);

        }, 0);

    }
}


function scriptEnding() {
    Messages.sendMessage('Hifi-Hand-Disabler', 'none');
    mapping.disable();
    for (var i = 0; i < pointers.length; ++i) {
        Overlays.deleteOverlay(pointers[i]);
    }
    MyAvatar.detachOne(GUN_MODEL);
    MyAvatar.detachOne(GUN_MODEL);
    clearPose();
}

MyAvatar.attach(GUN_MODEL, "LeftHand", GUN_OFFSETS[0], GUN_ORIENTATIONS[0], 0.40);
MyAvatar.attach(GUN_MODEL, "RightHand", GUN_OFFSETS[1], GUN_ORIENTATIONS[1], 0.40);

function showPointer(side) {
    Overlays.editOverlay(pointers[side], {
        visible: true
    });
}



mapping.from(Controller.Standard.LT).hysteresis(0.0, 0.5).to(function(value) {
    fire(0, value);
});


mapping.from(Controller.Standard.RT).hysteresis(0.0, 0.5).to(function(value) {
    fire(1, value);
});
mapping.enable();

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);


function createEntityHitEffect(position) {
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


function createBloodSplatter(position) {
        var splatter = Entities.addEntity({
        type: "ParticleEffect",
        position: position,
        lifetime: 4,
        "name": "Blood Splatter",
        "color": {
            red: 230,
            green: 2,
            blue: 30
        },
        "maxParticles": 1000,
        "lifespan": 0.3,
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
            "y": -5,
            "z": 0
        },
        "accelerationSpread": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "particleRadius": 0.05,
        "radiusSpread": 0.03,
        "radiusStart": 0.05,
        "radiusFinish": 0.05,
        "colorSpread": {
            red: 40,
            green: 0,
            blue: 30
        },
        "alpha": 1,
        "alphaSpread": 0,
        "alphaStart": 0,
        "alphaFinish": 0,
        "textures": "http://ericrius1.github.io/PartiArt/assets/star.png"
    });

    Script.setTimeout(function() {
        Entities.editEntity(splatter, {
            isEmitting: false
        });
    }, 100)

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
        });
    }, 100);

    var flash = Entities.addEntity({
        type: "ParticleEffect",
        position: position,
        lifetime: 4,
        "name": "Muzzle Flash",
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
