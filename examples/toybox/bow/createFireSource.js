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

function createFireSource() {

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
        position: this.bowProperties.position,
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

}

createFireSource();
