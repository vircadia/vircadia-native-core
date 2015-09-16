//
//  particlesTest.js
//  examples/example/entities
//
//  Created by David Rowe on 2 Sep 2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Click on the box entity to display different particle effects.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var box,
        particles,
        particleExample = -1,
        NUM_PARTICLE_EXAMPLES = 11,
        PARTICLE_RADIUS = 0.04;

    function onClickDownOnEntity(entityID) {
        if (entityID === box || entityID === particles) {
            particleExample = (particleExample + 1) % NUM_PARTICLE_EXAMPLES;

            switch (particleExample) {
            case 0:
                print("Simple emitter");
                Entities.editEntity(particles, {
                    speedSpread: 0.0,
                    accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
                    radiusSpread: 0.0,
                    animationIsPlaying: true
                });
                break;
            case 1:
                print("Speed spread");
                Entities.editEntity(particles, {
                    speedSpread: 0.1
                });
                break;
            case 2:
                print("Acceleration spread");
                Entities.editEntity(particles, {
                    speedSpread: 0.0,
                    accelerationSpread: { x: 0.0, y: 0.1, z: 0.0 }
                });
                break;
            case 3:
                print("Radius spread");
                Entities.editEntity(particles, {
                    accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
                    radiusSpread: 0.035
                });
                break;
            case 4:
                print("Radius start and finish");
                Entities.editEntity(particles, {
                    radiusSpread: 0.0,
                    radiusStart: 0.0,
                    radiusFinish: 0.0
                });
                break;
            case 5:
                print("Alpha 0.5");
                Entities.editEntity(particles, {
                    radiusStart: PARTICLE_RADIUS,
                    radiusFinish: PARTICLE_RADIUS,
                    alpha: 0.5
                });
                break;
            case 6:
                print("Alpha spread");
                Entities.editEntity(particles, {
                    alpha: 0.5,
                    alphaSpread: 0.5
                });
                break;
            case 7:
                print("Alpha start and finish");
                Entities.editEntity(particles, {
                    alphaSpread: 0.0,
                    alpha: 1.0,
                    alphaStart: 0.0,
                    alphaFinish: 0.0
                });
                break;
            case 8:
                print("Color spread");
                Entities.editEntity(particles, {
                    alpha: 1.0,
                    alphaStart: 1.0,
                    alphaFinish: 1.0,
                    color: { red: 128, green: 128, blue: 128 },
                    colorSpread: { red: 128, green: 0, blue: 0 }
                });
                break;
            case 9:
                print("Color start and finish");
                Entities.editEntity(particles, {
                    color: { red: 255, green: 255, blue: 255 },
                    colorSpread: { red: 0, green: 0, blue: 0 },
                    colorStart: { red: 255, green: 0, blue: 0 },
                    colorFinish: { red: 0, green: 255, blue: 0 }
                });
                break;
            case 10:
                print("Stop emitting");
                Entities.editEntity(particles, {
                    colorStart: { red: 255, green: 255, blue: 255 },
                    colorFinish: { red: 255, green: 255, blue: 255 },
                    animationIsPlaying: false
                });
                break;
            }
        }
    }

    function setUp() {
        var spawnPoint,
            boxPoint,
            animation = {
                fps: 30,
                frameIndex: 0,
                running: true,
                firstFrame: 0,
                lastFrame: 30,
                loop: true
            };

        boxPoint = Vec3.sum(MyAvatar.position, Vec3.multiply(4.0, Quat.getFront(Camera.getOrientation())));
        boxPoint = Vec3.sum(boxPoint, { x: 0.0, y: -0.5, z: 0.0 });
        spawnPoint = Vec3.sum(boxPoint, { x: 0.0, y: 0.75, z: 0.0 });

        box = Entities.addEntity({
            type: "Box",
            name: "ParticlesTest Box",
            position: boxPoint,
            dimensions: { x: 0.3, y: 0.3, z: 0.3 },
            color: { red: 128, green: 128, blue: 128 },
            lifetime: 3600  // 1 hour; just in case
        });

        particles = Entities.addEntity({
            type: "ParticleEffect",
            name: "ParticlesTest Effect",
            position: spawnPoint,
            particleRadius: PARTICLE_RADIUS,
            radiusSpread: 0.0,
            emitRate: 2.0,
            emitSpeed: 1.0,
            speedSpread: 0.0,
            emitAcceleration: { x: 0.0, y: -0.3, z: 0.0 },
            accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
            textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
            color: { red: 255, green: 255, blue: 255 },
            lifespan: 5.0,
            visible: true,
            locked: false,
            animationSettings: animation,
            animationIsPlaying: false,
            lifetime: 3600  // 1 hour; just in case
        });

        Entities.clickDownOnEntity.connect(onClickDownOnEntity);

        print("Click on the box to cycle through particle examples");
    }

    function tearDown() {
        Entities.clickDownOnEntity.disconnect(onClickDownOnEntity);
        Entities.deleteEntity(particles);
        Entities.deleteEntity(box);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());