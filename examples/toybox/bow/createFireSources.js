//
//  createFireSource.js
//
//  Created byJames Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a fire that you can use to light arrows on fire.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var TORCH_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/james/bow_and_arrow/models/torch.fbx';
var TORCH_DIMENSIONS = {
    x: 0.07,
    y: 1.6,
    z: 0.08
};

var FIRE_VERTICAL_OFFSET = 0.9;

function createFireSource(position) {

    var torchProperties = {
        type: 'Model',
        name: 'Hifi-Fire-Torch',
        modelURL: TORCH_MODEL_URL,
        shapeType: 'box',
        collisionsWillMove: false,
        ignoreForCollisions: true,
        dimensions: TORCH_DIMENSIONS,
        position: position
    };

    var torch = Entities.addEntity(torchProperties);
    torches.push(torch);
    var torchProperties = Entities.getEntityProperties(torch);

    var upVector = Quat.getUp(torchProperties.rotation);
    var upOffset = Vec3.multiply(upVector, FIRE_VERTICAL_OFFSET);
    var fireTipPosition = Vec3.sum(torchProperties.position, upOffset);

    var myOrientation = Quat.fromPitchYawRollDegrees(-90, 0, 0.0);

    var animationSettings = JSON.stringify({
        fps: 30,
        running: true,
        loop: true,
        firstFrame: 1,
        lastFrame: 10000
    });

    var fire = Entities.addEntity({
        type: "ParticleEffect",
        name: "Hifi-Arrow-Fire-Source",
        animationSettings: animationSettings,
        textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
        emitRate: 100,
        position: fireTipPosition,
        colorStart: {
            red: 70,
            green: 70,
            blue: 137
        },
        color: {
            red: 200,
            green: 99,
            blue: 42
        },
        colorFinish: {
            red: 255,
            green: 99,
            blue: 32
        },
        radiusSpread: 0.01,
        radiusStart: 0.02,
        radiusEnd: 0.001,
        particleRadius: 0.5,
        radiusFinish: 0.0,
        emitOrientation: myOrientation,
        emitSpeed: 0.3,
        speedSpread: 0.1,
        alphaStart: 0.05,
        alpha: 0.1,
        alphaFinish: 0.05,
        emitDimensions: {
            x: 1,
            y: 1,
            z: 0.1
        },
        polarFinish: 0.1,
        emitAcceleration: {
            x: 0.0,
            y: 0.0,
            z: 0.0
        },
        accelerationSpread: {
            x: 0.1,
            y: 0.01,
            z: 0.1
        },
        lifespan: 1
    });

    fires.push(fire)
}

var fireSourcePositions = [{
        x: 100,
        y: -1,
        z: 100
    }, {
        x: 100,
        y: -1,
        z: 102
    }, {
        x: 100,
        y: -1,
        z: 104
    }

];

var fires = [];
var torches = [];

fireSourcePositions.forEach(function(position) {
    createFireSource(position);
})

function cleanup() {
    while (fires.length > 0) {
        Entities.deleteEntity(fires.pop());
    }
     while (torches.length > 0) {
        Entities.deleteEntity(torches.pop());
    }
}

Script.scriptEnding.connect(cleanup);