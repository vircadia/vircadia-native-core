
Script.include("../../libraries/utils.js");

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));
var modelURL = "file:///C:/Users/Eric/Desktop/RaveRoom.fbx?v1" + Math.random();

var raveRoom = Entities.addEntity({
    type: "Model",
    modelURL: modelURL,
    position: center,
    visible:false
});

var colorPalette = [{
    red: 250,
    green: 137,
    blue: 162
}, {
    red: 204,
    green: 244,
    blue: 249
}, {
    red: 146,
    green: 206,
    blue: 116
}, {
    red: 240,
    green: 87,
    blue: 129
}];


var containerBall = Entities.addEntity({
    type: "Sphere",
    position: center,
    dimensions: {x: .1, y: .1, z: .1},
    color: {red: 1500, green: 10, blue: 50},
    collisionsWillMove: true,
    userData: JSON.stringify({
        grabbableKey: {
            spatialKey: {
                relativePosition: {
                    x: 0,
                    y: 1,
                    z: 0
                }
            },
            invertSolidWhileHeld: true
        }
    })
      // gravity: {x: 0, y: -.1, z: 0}
});

var lightZone = Entities.addEntity({
    type: "Zone",
    shapeType: 'box',
    keyLightIntensity: 0.2,
    keyLightColor: {
        red: 50,
        green: 0,
        blue: 50
    },
    keyLightAmbientIntensity: .2,
    position: MyAvatar.position,
    dimensions: {
        x: 100,
        y: 100,
        z: 100
    }
});

var light = Entities.addEntity({
        type: 'Light',
        position: center,
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
    position: center,
    type: "ParticleEffect",
    parentID: containerBall,
    isEmitting: true,
    "name": "ParticlesTest Emitter",
    "colorStart": {red: 200, green: 20, blue: 40},
     color: {red: 10, green: 0, blue: 255},
    "colorFinish": {red: 250, green: 200, blue:255},
    "maxParticles": 100000,
    "lifespan": 2,
    "emitRate": 10000,
    "emitSpeed": .2,
    "speedSpread": .01,
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
        "x": .01,
        "y": .01,
        "z": .01
    },
    "particleRadius": 0.04,
    "radiusSpread": 0,
    "radiusStart": 0.05,
    "radiusFinish": 0.0003,
    "alpha": 0,
    "alphaSpread": .5,
    "alphaStart": 0,
    "alphaFinish": 0.5,
    "additiveBlending": 0,
    "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
    emitterShouldTrail: false
})



function cleanup() {
    Entities.deleteEntity(lightBall);
    Entities.deleteEntity(containerBall);
    Entities.deleteEntity(raveRoom);
    Entities.deleteEntity(lightZone)
    Entities.deleteEntity(light);
}

Script.scriptEnding.connect(cleanup);