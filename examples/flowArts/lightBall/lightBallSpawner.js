var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));

var lightBall = Entities.addEntity({
    position: center,
    type: "ParticleEffect",
    isEmitting: true,
    "name": "ParticlesTest Emitter",
    "colorStart": {red: 20, green: 20, blue: 255},
     color: {red: 10, green: 0, blue: 255},
    "colorFinish": {red: 250, green: 200, blue:255},
    // "maxParticles": 100000,
    "lifespan": 1,
    "emitRate": 1000,
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
}

Script.scriptEnding.connect(cleanup);