Script.include("../../libraries/utils.js");

LightBall = function(spawnPosition) {

    var colorPalette = [{
        red: 25,
        green: 20,
        blue: 162
    }];


    var containerBall = Entities.addEntity({
        type: "Sphere",
        position: Vec3.sum(spawnPosition, {
            x: 0,
            y: .5,
            z: 0
        }),
        dimensions: {
            x: .1,
            y: .1,
            z: .1
        },
        color: {
            red: 15,
            green: 10,
            blue: 150
        },
        collisionsWillMove: true,
        gravity: {x: 0, y: -.5, z: 0},
        userData: JSON.stringify({
            grabbableKey: {
                spatialKey: {
                    relativePosition: {
                        x: 0,
                        y: .1,
                        z: 0
                    }
                },
                invertSolidWhileHeld: true
            }
        })
    });


    var light = Entities.addEntity({
        type: 'Light',
        parentID: containerBall,
        dimensions: {
            x: 30,
            y: 30,
            z: 30
        },
        color: colorPalette[randInt(0, colorPalette.length)],
        intensity: 5
    });


    var lightBall = Entities.addEntity({
        type: "ParticleEffect",
        parentID: containerBall,
        isEmitting: true,
        "name": "ParticlesTest Emitter",
        "colorStart": {
            red: 200,
            green: 20,
            blue: 40
        },
        color: {
            red: 200,
            green: 200,
            blue: 255
        },
        "colorFinish": {
            red: 25,
            green: 20,
            blue: 255
        },
        "maxParticles": 100000,
        "lifespan": 2,
        "emitRate": 10000,
        "emitSpeed": .1,
        "speedSpread": 0.0,
        "emitDimensions": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "polarStart": 0,
        "polarFinish": Math.PI,
        "azimuthStart": -Math.PI,
        "azimuthFinish": Math.PI,
        "emitAcceleration": {
            "x": 0,
            "y": 0,
            "z": 0
        },
        "accelerationSpread": {
            "x": .00,
            "y": .00,
            "z": .00
        },
        "particleRadius": 0.02,
        "radiusSpread": 0,
        "radiusStart": 0.03,
        "radiusFinish": 0.0003,
        "alpha": 0,
        "alphaSpread": .5,
        "alphaStart": 0,
        "alphaFinish": 0.5,
        // "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
        "textures": "file:///C:/Users/Eric/Desktop/Particle-Sprite-Smoke-1.png?v1" + Math.random(),
        emitterShouldTrail: true
    })



    function cleanup() {
        Entities.deleteEntity(lightBall);
        Entities.deleteEntity(containerBall);
        Entities.deleteEntity(light);
    }

    this.cleanup = cleanup;
}