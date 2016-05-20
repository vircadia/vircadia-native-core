//
//  RaveStick.js
//  examples/flowArats/raveStick
//
//  Created by Eric Levin on 12/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a rave stick which makes pretty light trails as you paint
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("../../libraries/utils.js");
var modelURL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/raveStick.fbx";
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
        dynamic: true,
        script: scriptURL,
        dimensions: {
            x: 0.06,
            y: 0.06,
            z: 0.31
        },
        userData: JSON.stringify({
            grabbableKey: {
                spatialKey: {
                    rightRelativePosition: {
                        x: 0.02,
                        y: 0,
                        z: 0
                    },
                    leftRelativePosition: {
                        x: -0.02,
                        y: 0,
                        z: 0
                    },
                    relativeRotation: Quat.fromPitchYawRollDegrees(90, 90, 0)
                },
                invertSolidWhileHeld: true
            }
        })
    });

    var glowEmitter = createGlowEmitter();

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



    function cleanup() {
        Entities.deleteEntity(stick);
        Entities.deleteEntity(light);
        Entities.deleteEntity(glowEmitter);
    }

    this.cleanup = cleanup;

    function createGlowEmitter() {
        var props = Entities.getEntityProperties(stick, ["position", "rotation"]);
        var forwardVec = Quat.getFront(props.rotation);
        var forwardQuat = Quat.rotationBetween(Vec3.UNIT_Z, forwardVec);
        var position = props.position;
        var color = {
            red: 150,
            green: 20,
            blue: 100
        }
        var props = {
            type: "ParticleEffect",
            name: "Rave Stick Glow Emitter",
            position: position,
            parentID: stick,
            isEmitting: true,
            colorStart: color,
            color: {
                red: 200,
                green: 200,
                blue: 255
            },
            colorFinish: color,
            maxParticles: 100000,
            lifespan: 0.8,
            emitRate: 1000,
            emitOrientation: forwardQuat,
            emitSpeed: 0.2,
            speedSpread: 0.0,
            emitDimensions: {
                x: 0,
                y: 0,
                z: 0
            },
            polarStart: 0,
            polarFinish: 0,
            azimuthStart: 0.1,
            azimuthFinish: 0.01,
            emitAcceleration: {
                x: 0,
                y: 0,
                z: 0
            },
            accelerationSpread: {
                x: 0.00,
                y: 0.00,
                z: 0.00
            },
            radiusStart: 0.01,
            radiusFinish: 0.005,
            alpha: 0.7,
            alphaSpread: 0.1,
            alphaStart: 0.1,
            alphaFinish: 0.1,
            textures: "https://s3.amazonaws.com/hifi-public/eric/textures/particleSprites/beamParticle.png",
            emitterShouldTrail: false
        }
        var glowEmitter = Entities.addEntity(props);
        return glowEmitter;

    }
}
