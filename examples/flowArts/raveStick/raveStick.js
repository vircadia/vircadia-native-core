//
//  RaveStick.js
//  examples/flowArats/raveStick
//
//  Created by Eric Levin on 12/17/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  This script creates a rave stick which makes pretty light trails as you paint
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/utils.js");
var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/rave/raveStick.fbx";
var scriptURL = Script.resolvePath("raveStickEntityScript.js");
RaveStick = function(spawnPosition) {
    var colorPalette = [{
        red: 0,
        green: 200,
        blue: 40
    }, {
        red: 200,
        green: 10,
        blue: 40
    }];
    var stick = Entities.addEntity({
        type: "Model",
        name: "raveStick",
        modelURL: modelURL,
        position: spawnPosition,
        shapeType: 'box',
        script: scriptURL,
        dimensions: {
            x: 0.06,
            y: 0.06,
            z: 0.31
        },
        userData: JSON.stringify({
            grabbableKey: {
                spatialKey: {
                    relativePosition: {
                        x: 0,
                        y: 0,
                        z: -0.1
                    },
                    relativeRotation: Quat.fromPitchYawRollDegrees(90, 90, 0)
                },
                invertSolidWhileHeld: true
            }
        })
    });

    var light = Entities.addEntity({
        type: 'Light',
        name: "raveLight",
        parentID: stick,
        dimensions: {
            x: 30,
            y: 30,
            z: 30
        },
        color: colorPalette[randInt(0, colorPalette.length)],
        intensity: 5
    });

    var rotation = Quat.fromPitchYawRollDegrees(0, 0, 0)
    var forwardVec = Quat.getFront(Quat.multiply(rotation, Quat.fromPitchYawRollDegrees(-90, 0, 0)));
    forwardVec = Vec3.normalize(forwardVec);
    var forwardQuat = orientationOf(forwardVec);
    var position = Vec3.sum(spawnPosition, Vec3.multiply(Quat.getFront(rotation), 0.1));
    position.z += 0.1;
    position.x += -0.035;
    var color = {
        red: 0,
        green: 200,
        blue: 40
    };
    var props = {
        type: "ParticleEffect",
        position: position,
        parentID: stick,
        isEmitting: true,
        name: "raveBeam",
        colorStart: color,
        colorSpread: {
            red: 200,
            green: 10,
            blue: 10
        },
        color: {
            red: 200,
            green: 200,
            blue: 255
        },
        colorFinish: color,
        maxParticles: 100000,
        lifespan: 1,
        emitRate: 1000,
        emitOrientation: forwardQuat,
        emitSpeed: .2,
        speedSpread: 0.0,
        polarStart: 0,
        polarFinish: .0,
        azimuthStart: .1,
        azimuthFinish: .01,
        emitAcceleration: {
            x: 0,
            y: 0,
            z: 0
        },
        accelerationSpread: {
            x: .00,
            y: .00,
            z: .00
        },
        radiusStart: 0.03,
        radiusFinish: 0.025,
        alpha: 0.7,
        alphaSpread: .1,
        alphaStart: 0.5,
        alphaFinish: 0.5,
        textures: "https://s3.amazonaws.com/hifi-public/eric/textures/particleSprites/beamParticle.png",
        emitterShouldTrail: false,
    }
    var beam = Entities.addEntity(props);



    function cleanup() {
        Entities.deleteEntity(stick);
        Entities.deleteEntity(light);
        Entities.deleteEntity(beam);
    }

    this.cleanup = cleanup;
}