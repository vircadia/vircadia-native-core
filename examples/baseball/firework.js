//
//  firework.js
//  examples/baseball/
//
//  Created by Ryan Huffman on Nov 9, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("utils.js");

var emitters = [];

var smokeTrailSettings = {
    "name":"ParticlesTest Emitter",
    "type": "ParticleEffect",
    "color":{"red":205,"green":84.41176470588235,"blue":84.41176470588235},
    "maxParticles":1000,
    "velocity": { x: 0, y: 18.0, z: 0 },
    "lifetime": 20,
    "lifespan":3,
    "emitRate":100,
    "emitSpeed":0.5,
    "speedSpread":0,
    "emitOrientation":{"x":0,"y":0,"z":0,"w":1},
    "emitDimensions":{"x":0,"y":0,"z":0},
    "emitRadiusStart":0.5,
    "polarStart":1,
    "polarFinish":1,
    "azimuthStart":0,
    "azimuthFinish":0,
    "emitAcceleration":{"x":0,"y":-0.70000001192092896,"z":0},
    "accelerationSpread":{"x":0,"y":0,"z":0},
    "particleRadius":0.03999999910593033,
    "radiusSpread":0,
    "radiusStart":0.13999999910593033,
    "radiusFinish":0.14,
    "colorSpread":{"red":0,"green":0,"blue":0},
    "colorStart":{"red":255,"green":255,"blue":255},
    "colorFinish":{"red":255,"green":255,"blue":255},
    "alpha":1,
    "alphaSpread":0,
    "alphaStart":0,
    "alphaFinish":1,
    "textures":"https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png"
};

var fireworkSettings = {
    "name":"ParticlesTest Emitter",
    "type": "ParticleEffect",
    "color":{"red":205,"green":84.41176470588235,"blue":84.41176470588235},
    "maxParticles":1000,
    "lifetime": 20,
    "lifespan":4,
    "emitRate":1000,
    "emitSpeed":1.5,
    "speedSpread":1.0,
    "emitOrientation":{"x":-0.2,"y":0,"z":0,"w":0.7000000000000001},
    "emitDimensions":{"x":0,"y":0,"z":0},
    "emitRadiusStart":0.5,
    "polarStart":1,
    "polarFinish":1.2,
    "azimuthStart":-Math.PI,
    "azimuthFinish":Math.PI,
    "emitAcceleration":{"x":0,"y":-0.70000001192092896,"z":0},
    "accelerationSpread":{"x":0,"y":0,"z":0},
    "particleRadius":0.03999999910593033,
    "radiusSpread":0,
    "radiusStart":0.13999999910593033,
    "radiusFinish":0.14,
    "colorSpread":{"red":0,"green":0,"blue":0},
    "colorStart":{"red":255,"green":255,"blue":255},
    "colorFinish":{"red":255,"green":255,"blue":255},
    "alpha":1,
    "alphaSpread":0,
    "alphaStart":0,
    "alphaFinish":1,
    "textures":"https://hifi-public.s3.amazonaws.com/alan/Particles/spark_2.png",
};

var popSounds = getSounds([
    "atp:a2bf79c95fe74c2c6c9188acc7230f7cd1b0f6008f2c81954ecd93eca0497ec6.wav",
    "atp:901067ebc2cda4c0d86ec02fcca2ed901e85f9097ad68bbde78b4cad8eaf2ed7.wav",
    "atp:830312930577cb1ea36ba2d743e957debbacceb441b20addead5a6faa05a3771.wav",
    "atp:62e80d0a9f084cf731bcc66ca6e9020ee88587417071a281eee3167307b53560.wav"
]);

var launchSounds = getSounds([
    "atp:ee6afe565576c4546c6d6cd89c1af532484c9b60ab30574d6b40c2df022f7260.wav",
    "atp:91ef19ba1c78be82d3fd06530cd05ceb90d1e75f4204c66819c208c55da049ef.wav",
    "atp:ee56993daf775012cf49293bfd5971eec7e5c396642f8bfbea902ba8f47b56cd.wav",
    "atp:37775d267f00f82242a7e7f61f3f3d7bf64a54c5a3799e7f2540fa5f6b79bd02.wav"
]);

function playRandomSound(sounds, options) {
    Audio.playSound(sounds[randomInt(sounds.length)], options);
}

function shootFirework(position, color, options) {
    smokeTrailSettings.position = position;
    smokeTrailSettings.velocity = randomVec3(-5, 5, 10, 20, 10, 15);
    smokeTrailSettings.gravity = randomVec3(-5, 5, -9.8, -9.8, 20, 40);

    playRandomSound(launchSounds, { position: {x: 0, y: 0 , z: 0}, volume: 3.0 });
    var smokeID = Entities.addEntity(smokeTrailSettings);

    Script.setTimeout(function() {
        Entities.editEntity(smokeID, { emitRate: 0 });
        var position = Entities.getEntityProperties(smokeID, ['position']).position;
        fireworkSettings.position = position;
        fireworkSettings.colorStart = color;
        fireworkSettings.colorFinish = color;
        var burstID = Entities.addEntity(fireworkSettings);
        playRandomSound(popSounds, { position: {x: 0, y: 0 , z: 0}, volume: 3.0 });
        Script.setTimeout(function() {
            Entities.editEntity(burstID, { emitRate: 0 });
        }, 500);
        Script.setTimeout(function() {
            Entities.deleteEntity(smokeID);
            Entities.deleteEntity(burstID);
        }, 10000);
    }, 2000);
}

playFireworkShow = function(position, numberOfFireworks, duration) {
    for (var i = 0; i < numberOfFireworks; i++) {
        var randomOffset = randomVec3(-15, 15, -3, 3, -1, 1);
        var randomPosition = Vec3.sum(position, randomOffset);
        Script.setTimeout(function(position) {
            return function() {
                var color = randomColor(128, 255, 128, 255, 128, 255);
                shootFirework(position, color, fireworkSettings);
            }
        }(randomPosition), Math.random() * duration)
    }
}
