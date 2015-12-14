var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

var containerBall = Entities.addEntity({
    type: "Sphere",
    position: center,
    dimensions: {x: .1, y: .1, z: .1},
    color: {red: 50, green: 10, blue: 50},
    collisionsWillMove: true,
      gravity: {x: 0, y: -.1, z: 0}
});

var lightBall = Entities.addEntity({
    position: center,
    type: "ParticleEffect",
    parentID: containerBall,
    isEmitting: true,
    "name": "ParticlesTest Emitter",
    "colorStart": {red: 20, green: 20, blue: 255},
     color: {red: 10, green: 0, blue: 255},
    "colorFinish": {red: 250, green: 200, blue:255},
    "maxParticles": 20000,
    "lifespan": 1,
    "emitRate": 10000,
    "emitSpeed": .1,
    "speedSpread": .01,
    "emitDimensions": {
        "x": 0,
        "y": 0,
        "z": 0
    },
    "polarStart": 0,
    "polarFinish": 3,
    "azimuthStart": -3.1415927410125732,
    "azimuthFinish": 3.1415927410125732,
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
    "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png"
})

var containerBall2 = Entities.addEntity({
    type: "Sphere",
    position: Vec3.sum(center, {x: 0.5, y: 0, z: 0}),
    dimensions: {x: .1, y: .1, z: .1},
    color: {red: 200, green: 10, blue: 50},
    collisionsWillMove: true,
    gravity: {x: 0, y: -.1, z: 0}
});

var lightBall2 = Entities.addEntity({
    position: Vec3.sum(center, {x: 0.5, y: 0, z: 0}),
    type: "ParticleEffect",
    parentID: containerBall2,
    isEmitting: true,
    "name": "ParticlesTest Emitter",
    "colorStart": {red: 200, green: 20, blue: 0},
     color: {red: 255, green: 0, blue: 10},
    "colorFinish": {red: 250, green: 200, blue:255},
    "maxParticles": 100000,
    "lifespan": 1,
    "emitRate": 10000,
    "emitSpeed": .1,
    "speedSpread": .01,
    "emitDimensions": {
        "x": 0,
        "y": 0,
        "z": 0
    },
    "polarStart": 0,
    "polarFinish": 3,
    "azimuthStart": -3.1415927410125732,
    "azimuthFinish": 3.1415927410125732,
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
    "textures": "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png"
})

function cleanup() {
    Entities.deleteEntity(lightBall);
    Entities.deleteEntity(lightBall2);
    Entities.deleteEntity(containerBall);
    Entities.deleteEntity(containerBall2);
}

Script.scriptEnding.connect(cleanup);