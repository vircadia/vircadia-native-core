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
        sphere,
        sphereDimensions = { x: 0.4, y: 0.8, z: 0.2 },
        pointDimensions = { x: 0.0, y: 0.0, z: 0.0 },
        sphereOrientation = Quat.fromPitchYawRollDegrees(-60.0, 30.0, 0.0),
        verticalOrientation = Quat.fromPitchYawRollDegrees(-90.0, 0.0, 0.0),
        particles,
        particleExample = -1,
        PARTICLE_RADIUS = 0.04,
        SLOW_EMIT_RATE = 2.0,
        HALF_EMIT_RATE = 50.0,
        FAST_EMIT_RATE = 100.0,
        SLOW_EMIT_SPEED = 0.025,
        FAST_EMIT_SPEED = 1.0,
        GRAVITY_EMIT_ACCELERATON = { x: 0.0, y: -0.3, z: 0.0 },
        ZERO_EMIT_ACCELERATON = { x: 0.0, y: 0.0, z: 0.0 },
        PI = 3.141592,
        DEG_TO_RAD = PI / 180.0,
        NUM_PARTICLE_EXAMPLES = 18;

    function onClickDownOnEntity(entityID) {
        if (entityID === box || entityID === sphere || entityID === particles) {
            particleExample = (particleExample + 1) % NUM_PARTICLE_EXAMPLES;

            switch (particleExample) {
            case 0:
                print("Simple emitter");
                Entities.editEntity(particles, {
                    speedSpread: 0.0,
                    accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
                    radiusSpread: 0.0,
                    isEmitting: true
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
                print("Radius spread - temporarily not working");
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
                    particleRadius: PARTICLE_RADIUS,
                    radiusFinish: 0.0
                });
                break;
            case 5:
                print("Alpha 0.5");
                Entities.editEntity(particles, {
                    radiusStart: PARTICLE_RADIUS,
                    particleRadius: PARTICLE_RADIUS,
                    radiusFinish: PARTICLE_RADIUS,
                    alpha: 0.5
                });
                break;
            case 6:
                print("Alpha spread - temporarily not working");
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
                print("Color spread - temporarily not working");
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
                print("Emit in a spread beam");
                Entities.editEntity(particles, {
                    colorStart: { red: 255, green: 255, blue: 255 },
                    colorFinish: { red: 255, green: 255, blue: 255 },
                    alphaFinish: 0.0,
                    emitRate: FAST_EMIT_RATE,
                    polarFinish: 2.0 * DEG_TO_RAD
                });
                break;
            case 11:
                print("Emit in all directions from point");
                Entities.editEntity(particles, {
                    emitSpeed: SLOW_EMIT_SPEED,
                    emitAcceleration: ZERO_EMIT_ACCELERATON,
                    polarFinish: PI
                });
                break;
            case 12:
                print("Emit from sphere surface");
                Entities.editEntity(particles, {
                    colorStart: { red: 255, green: 255, blue: 255 },
                    colorFinish: { red: 255, green: 255, blue: 255 },
                    emitDimensions: sphereDimensions,
                    emitOrientation: sphereOrientation
                });
                Entities.editEntity(box, {
                    visible: false
                });
                Entities.editEntity(sphere, {
                    visible: true
                });
                break;
            case 13:
                print("Emit from hemisphere of sphere surface");
                Entities.editEntity(particles, {
                    polarFinish: PI / 2.0
                });
                break;
            case 14:
                print("Emit from equator of sphere surface");
                Entities.editEntity(particles, {
                    polarStart: PI / 2.0,
                    emitRate: HALF_EMIT_RATE
                });
                break;
            case 15:
                print("Emit from half equator of sphere surface");
                Entities.editEntity(particles, {
                    azimuthStart: -PI / 2.0,
                    azimuthFinish: PI / 2.0
                });
                break;
            case 16:
                print("Emit within eighth of sphere volume");
                Entities.editEntity(particles, {
                    polarStart: 0.0,
                    polarFinish: PI / 2.0,
                    azimuthStart: 0,
                    azimuthFinish: PI / 2.0,
                    emitRadiusStart: 0.0,
                    alphaFinish: 1.0,
                    emitSpeed: 0.0
                });
                break;
            case 17:
                print("Stop emitting");
                Entities.editEntity(particles, {
                    emitDimensions: pointDimensions,
                    emitOrientation: verticalOrientation,
                    alphaFinish: 1.0,
                    emitRate: SLOW_EMIT_RATE,
                    emitSpeed: FAST_EMIT_SPEED,
                    emitAcceleration: GRAVITY_EMIT_ACCELERATON,
                    polarStart: 0.0,
                    polarFinish: 0.0,
                    azimuthStart: -PI,
                    azimuthFinish: PI,
                    isEmitting: false
                });
                Entities.editEntity(box, {
                    visible: true
                });
                Entities.editEntity(sphere, {
                    visible: false
                });
                break;
            }
        }
    }

    function setUp() {
        var boxPoint,
            spawnPoint;

        boxPoint = Vec3.sum(MyAvatar.position, Vec3.multiply(4.0, Quat.getFront(Camera.getOrientation())));
        boxPoint = Vec3.sum(boxPoint, { x: 0.0, y: -0.5, z: 0.0 });
        spawnPoint = Vec3.sum(boxPoint, { x: 0.0, y: 1.0, z: 0.0 });

        box = Entities.addEntity({
            type: "Box",
            name: "ParticlesTest Box",
            position: boxPoint,
            rotation: verticalOrientation,
            dimensions: { x: 0.3, y: 0.3, z: 0.3 },
            color: { red: 128, green: 128, blue: 128 },
            lifetime: 3600,  // 1 hour; just in case
            visible: true
        });

        // Same size and orientation as emitter when ellipsoid.
        sphere = Entities.addEntity({
            type: "Sphere",
            name: "ParticlesTest Sphere",
            position: boxPoint,
            rotation: sphereOrientation,
            dimensions: sphereDimensions,
            color: { red: 128, green: 128, blue: 128 },
            lifetime: 3600,  // 1 hour; just in case
            visible: false
        });

        // 1.0m above the box or ellipsoid.
        particles = Entities.addEntity({
            type: "ParticleEffect",
            name: "ParticlesTest Emitter",
            position: spawnPoint,
            emitOrientation: verticalOrientation,
            particleRadius: PARTICLE_RADIUS,
            radiusSpread: 0.0,
            emitRate: SLOW_EMIT_RATE,
            emitSpeed: FAST_EMIT_SPEED,
            speedSpread: 0.0,
            emitAcceleration: GRAVITY_EMIT_ACCELERATON,
            accelerationSpread: { x: 0.0, y: 0.0, z: 0.0 },
            textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
            color: { red: 255, green: 255, blue: 255 },
            lifespan: 5.0,
            locked: false,
            isEmitting: false,
            lifetime: 3600  // 1 hour; just in case
        });

        Entities.clickDownOnEntity.connect(onClickDownOnEntity);

        print("Click on the box to cycle through particle examples");
    }

    function tearDown() {
        Entities.clickDownOnEntity.disconnect(onClickDownOnEntity);
        Entities.deleteEntity(particles);
        Entities.deleteEntity(box);
        Entities.deleteEntity(sphere);
    }

    setUp();
    Script.scriptEnding.connect(tearDown);
}());