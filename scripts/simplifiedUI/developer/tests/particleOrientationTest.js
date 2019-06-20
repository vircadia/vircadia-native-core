//
//  particleOrientationTest.js
//  examples/tests
//
//  Created by Piper.Peppercorn.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var emitterBone = 'Head'
var particleEntities = [];

function emitter(jointName) {
    var jointID = MyAvatar.jointNames.indexOf(jointName);
    var newEmitter = Entities.addEntity({
        name: 'particleEmitter ' + jointName,
        type: 'ParticleEffect',
        emitterShouldTrail: true,
        textures: 'https://dl.dropboxusercontent.com/u/96759331/ParticleTest.png',
        position: Vec3.sum(MyAvatar.getAbsoluteJointRotationInObjectFrame(jointID), MyAvatar.position),
        parentJointIndex: jointID,
        position: MyAvatar.getJointPosition(jointName),
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        isEmitting: 1,
        maxParticles: 1,
        lifespan: 2.0
        ,
        emitRate: 1,
        emitSpeed: 0.0,
        speedSpread: 0.0,
        /*
        emitOrientation: {
            x: -0.7035577893257141,
            y: -0.000015259007341228426,
            z: -0.000015259007341228426,
            w: 1.7106381058692932
        },
        */
        emitOrientation: {
            x:0,
            y: 0,
            z: 0,
            w: 1
        },
        emitRadiusStart: 0,
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
        particleRadius: 2.0,
        radiusSpread: 1.0,
        radiusStart: 2.0,
        radiusFinish: 2.0,
        colorSpread: {
            red: 0,
            green: 0,
            blue: 0
        },
        colorStart: {
            red: 255,
            green: 255,
            blue: 255
        },
        colorFinish: {
            red: 255,
            green: 255,
            blue: 255
        },
        alpha: 1,
        alphaSpread: 0,
        alphaStart: 1,
        alphaFinish: 1
    });
    return newEmitter;
}


Script.scriptEnding.connect(function() {
    for (var i = 0; i < particleEntities.length; i++) {
        // Fixes a crash on shutdown:
        Entities.editEntity(particleEntities[i], { parentID: '' });
        Entities.deleteEntity(particleEntities[i]);
    }
});

particleEntities.push(emitter(emitterBone));
