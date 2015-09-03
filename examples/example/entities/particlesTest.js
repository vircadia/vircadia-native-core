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
        NUM_PARTICLE_EXAMPLES = 4;

    function onClickDownOnEntity(entityID) {
        if (entityID === box || entityID === particles) {
            particleExample = (particleExample + 1) % NUM_PARTICLE_EXAMPLES;

            switch (particleExample) {
            case 0:
                print("Simple animation");
                Entities.editEntity(particles, {
                    velocitySpread: { x: 0.0, y: 0.0, z: 0.0 },
                    accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
                    radiusSpread: 0.0,
                    animationIsPlaying: true
                });
                break;
            case 1:
                print("Velocity spread");
                Entities.editEntity(particles, {
                    velocitySpread: { x: 0.1, y: 0.0, z: 0.1 },
                    accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
                    radiusSpread: 0.0,
                    animationIsPlaying: true
                });
                break;
            case 2:
                print("Acceleration spread");
                Entities.editEntity(particles, {
                    velocitySpread: { x: 0.0, y: 0.0, z: 0.0 },
                    accelerationSpread: { x: 0.0, y: 0.1, z: 0.0 },
                    radiusSpread: 0.0,
                    animationIsPlaying: true
                });
                break;
            case 3:
                print("Radius spread");
                Entities.editEntity(particles, {
                    velocitySpread: { x: 0.0, y: 0.0, z: 0.0 },
                    accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
                    radiusSpread: 0.035,
                    animationIsPlaying: true
                });
                break;
            }
        }
    }

    function setUp() {
        var spawnPoint = Vec3.sum(MyAvatar.position, Vec3.multiply(4.0, Quat.getFront(Camera.getOrientation()))),
            animation = {
                fps: 30,
                frameIndex: 0,
                running: true,
                firstFrame: 0,
                lastFrame: 30,
                loop: true
            };

        box = Entities.addEntity({
            type: "Box",
            position: spawnPoint,
            dimensions: { x: 0.3, y: 0.3, z: 0.3 },
            color: { red: 128, green: 128, blue: 128 },
            lifetime: 3600  // 1 hour; just in case
        });

        particles = Entities.addEntity({
            type: "ParticleEffect",
            position: spawnPoint,
            particleRadius: 0.04,
            particleRadiusSpread: 0.0,
            emitRate: 2.0,
            emitVelocity: { x: 0.0, y: 1.0, z: 0.0 },
            velocitySpread: { x: 0.0, y: 0.0, z: 0.0 },
            emitAcceleration: { x: 0.0, y: -0.3, z: 0.0 },
            accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
            textures: "https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png",
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