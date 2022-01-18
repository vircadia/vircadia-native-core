//
//
//  Created by Chris Collins on April 11, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Attaches particles to your finger joints. Lots of fun!



var handiness = 'both'; // left, right or both
var particleFingers = ['HandPinky4', 'HandRing4', 'HandIndex4', 'HandThumb4', 'HandMiddle4'];

var particleEntities = [];
var PARICLE_NAME_BASE = 'spawnedFingerParticle'

// what the actual particles look like
var particleProperties = {
    type: 'ParticleEffect',
    parentID: MyAvatar.sessionUUID,
    color: {
        red: 125,
        green: 125,
        blue: 125
    },
    isEmitting: 1,
    maxParticles: 1000,
    lifespan: 1,
    emitRate: 100,
    emitSpeed: 0,
    speedSpread: 0,
    emitOrientation: {
        x: -0.7035577893257141,
        y: -0.000015259007341228426,
        z: -0.000015259007341228426,
        w: 0.7106381058692932
    },
    emitRadiusStart: 1,
    polarStart: 0,
    polarFinish: 0,
    azimuthFinish: 3.1415927410125732,
    emitAcceleration: {
        x: 0,
        y: 0,
        z: 0
    },
    accelerationSpread: {
        x: 0,
        y: 0,
        z: 0
    },
    particleRadius: 0.004999999888241291,
    radiusSpread: 0,
    radiusStart: 0.0010000000474974513,
    radiusFinish: 0.0010000000474974513,
    colorSpread: {
        red: 125,
        green: 125,
        blue: 125
    },
    colorStart: {
        red: 125,
        green: 125,
        blue: 125
    },
    colorFinish: {
        red: 125,
        green: 125,
        blue: 125
    },
    alpha: 1,
    alphaSpread: 0,
    alphaStart: 1,
    alphaFinish: 0,
    emitterShouldTrail: true,
    textures: 'https://cdn-1.vircadia.com/us-e-1/Developer/Tutorials/particleFingers/smoke.png',
    lifetime: 3600
};

function createParticleAtFinger(jointName) {
    var jointID = MyAvatar.jointNames.indexOf(jointName);
    particleProperties.name = PARICLE_NAME_BASE + jointName; 
    particleProperties.parentJointIndex = jointID;
    position =  MyAvatar.getJointPosition(jointName);
    return Entities.addEntity(particleProperties);
}

function addParticlesForHand(handPrefix) {
    for (var i = 0; i < particleFingers.length; i++) {
        particleEntities.push(createParticleAtFinger(handPrefix + particleFingers[i]));
        print(handPrefix + particleFingers[i]);
    }
}

Script.scriptEnding.connect(function() {
    for (var i = 0; i < particleEntities.length; i++) {
        // Fixes a crash on shutdown:
        // Entities.editEntity(particleEntities[i], { parentID: '' });
        Entities.deleteEntity(particleEntities[i]);
    }
});


if (handiness === "both" || handiness === "left") {
    addParticlesForHand("Left");
}
if (handiness === "both" || handiness === "right") {
    addParticlesForHand("Right");
}