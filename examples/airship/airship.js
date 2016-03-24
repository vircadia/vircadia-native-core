//
//  airship.js
//
//  Animates a pirate airship that chases people and shoots cannonballs at them
//
//  Created by Philip Rosedale on March 7, 2016
//  Copyright 2016 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var entityID,
        minimumDelay = 100,       // milliseconds
        distanceScale = 2.0,      //  distance at which 100% chance of hopping
        timeScale = 300.0,          // crash
        hopStrength = 0.4,        //  meters / second
        spotlight = null,
        wantDebug = false,
        timeoutID = undefined,
        bullet = null,
        particles = null,
        nearbyAvatars = 0,
        nearbyAvatarsInRange = 0,
        averageAvatarLocation = { x: 0, y: 0, z: 0 },
        properties,
        lightTimer = 0,
        lightTimeoutID = undefined,
        audioInjector = null;

    var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
    var cannonSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "philip/cannonShot.wav");
    var explosionSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "philip/explosion.wav");

    var NO_SHOOT_COLOR = { red: 100, green: 100, blue: 100 };

    var MAX_TARGET_RANGE = 200; 

    function printDebug(message) {
        if (wantDebug) {
            print(message);
        }
    }

    var LIGHT_UPDATE_INTERVAL = 50;
    var LIGHT_LIFETIME = 700;

    function randomVector(size) {
        return { x: (Math.random() - 0.5) * size, 
                 y: (Math.random() - 0.5) * size,
                 z: (Math.random() - 0.5) * size };
    }

    function makeLight(parent, position, colorDivisor) {
        //  Create a flickering light somewhere for a while
        if (spotlight !== null) {
            // light still exists, do nothing
            printDebug("light still there");
            return;
        }
        printDebug("making light");

        var colorIndex = 180 + Math.random() * 50;

        spotlight = Entities.addEntity({
                    type: "Light",
                    name: "Test Light",
                    intensity: 50.0,
                    falloffRadius: 20.0,
                    dimensions: {
                        x: 150,
                        y: 150,
                        z: 150
                    },
                    position: position,
                    parentID: parent,
                    color: {
                        red: colorIndex,
                        green: colorIndex / colorDivisor,
                        blue: 0
                    },
                    lifetime:  LIGHT_LIFETIME * 2
                });

        lightTimer = 0;
        lightTimeoutID = Script.setTimeout(updateLight, LIGHT_UPDATE_INTERVAL);
        
    };

    function updateLight() {
        lightTimer += LIGHT_UPDATE_INTERVAL;
        if ((spotlight !== null) && (lightTimer > LIGHT_LIFETIME)) {
            printDebug("deleting light!");
            Entities.deleteEntity(spotlight);
            spotlight = null;
        } else {
            Entities.editEntity(spotlight, {
                intensity: 5 + Math.random() * 50,
                falloffRadius:  5 + Math.random() * 10 
            });
            lightTimeoutID = Script.setTimeout(updateLight, LIGHT_UPDATE_INTERVAL);
        }
    }

    function move() {
        
        var HOVER_DISTANCE = 30.0;
        var RUN_TOWARD_STRENGTH = 0.002;
        var VELOCITY_FOLLOW_RATE = 0.01;

        var range = Vec3.distance(properties.position, averageAvatarLocation);
        var impulse = { x: 0, y: 0, z: 0 };

        //  move in the XZ plane
        var away = Vec3.subtract(properties.position, averageAvatarLocation);
        away.y = 0.0;

        if (range > HOVER_DISTANCE) {
            impulse = Vec3.multiply(-RUN_TOWARD_STRENGTH, Vec3.normalize(away));
        }
        
        var rotation = Quat.rotationBetween(Vec3.UNIT_NEG_Z, properties.velocity);
        Entities.editEntity(entityID, {velocity: Vec3.sum(properties.velocity, impulse), rotation: Quat.mix(properties.rotation, rotation, VELOCITY_FOLLOW_RATE)});
    }

    
    function countAvatars() {
        var workQueue = AvatarList.getAvatarIdentifiers();
        var averageLocation = {x: 0, y: 0, z: 0};
        var summed = 0;
        var inRange = 0;
        for (var i = 0; i < workQueue.length; i++) {
            var avatar = AvatarList.getAvatar(workQueue[i]), avatarPosition = avatar && avatar.position;
            if (avatarPosition) {
                averageLocation = Vec3.sum(averageLocation, avatarPosition);
                summed++;
                if (Vec3.distance(avatarPosition, properties.position) < MAX_TARGET_RANGE) {
                    inRange++;
                }
            }
        }
        if (summed > 0) {
            averageLocation = Vec3.multiply(1 / summed, averageLocation);
        }
        nearbyAvatars = summed;
        nearbyAvatarsInRange = inRange;
        averageAvatarLocation = averageLocation;

        //printDebug("  Avatars: " + summed + "in range: " + nearbyAvatarsInRange);
        //Vec3.print(" location: ", averageAvatarLocation);

        return; 
    }

    function shoot() {
        if (bullet !== null) {
           return;
        }
        if (Vec3.distance(MyAvatar.position, properties.position) > MAX_TARGET_RANGE) {
            return;
        }

        var SPEED = 7.5;
        var GRAVITY = -2.0;
        var DIAMETER = 0.5;
        var direction = Vec3.subtract(MyAvatar.position, properties.position);
        var range = Vec3.distance(MyAvatar.position, properties.position);
        var timeOfFlight = range / SPEED;

        var fall = 0.5 * -GRAVITY * (timeOfFlight * timeOfFlight);

        var velocity = Vec3.multiply(SPEED, Vec3.normalize(direction));
        velocity.y += 0.5 * fall / timeOfFlight * 2.0;

        var DISTANCE_TO_DECK = 3;
        var bulletStart = properties.position;
        bulletStart.y -= DISTANCE_TO_DECK;

        makeLight(entityID, Vec3.sum(properties.position, randomVector(10.0)), 2);

        bullet = Entities.addEntity({ 
            type: "Sphere",
            name: "cannonball",
            position: properties.position,
            dimensions: { x: DIAMETER, y: DIAMETER, z: DIAMETER },
            color: { red: 10, green: 10, blue: 10 },
            velocity: velocity,
            damping: 0.01,
            dynamic: true,
            ignoreForCollisions: true,
            gravity: { x: 0, y: GRAVITY, z: 0 },
            lifetime: timeOfFlight * 2
        });

        Audio.playSound(cannonSound, {
            position: Vec3.sum(MyAvatar.position, velocity),
            volume: 1.0
        });
        makeParticles(properties.position, bullet, timeOfFlight * 2);
        Script.setTimeout(explode, timeOfFlight * 1000);
    }

    function explode() {
         var properties = Entities.getEntityProperties(bullet);
         var direction = Vec3.normalize(Vec3.subtract(MyAvatar.position, properties.position));
         makeLight(null, properties.position, 10);
         Audio.playSound(explosionSound, { position: Vec3.sum(MyAvatar.position, direction), volume: 1.0 }); 
         bullet = null;
    }


    function maybe() {  // every user checks their distance and tries to claim if close enough.
        var PROBABILITY_OF_SHOOT = 0.015;
        properties = Entities.getEntityProperties(entityID);
        countAvatars();

        if (nearbyAvatars && (Math.random() < 1 / nearbyAvatars)) {
            move();
        }

        if (nearbyAvatarsInRange && (Math.random() < PROBABILITY_OF_SHOOT / nearbyAvatarsInRange)) {
            shoot();
        }

        var TIME_TO_NEXT_CHECK = 33;
        timeoutID = Script.setTimeout(maybe, TIME_TO_NEXT_CHECK);
    }

    this.preload = function (givenEntityID) {
        printDebug("preload airship v1...");

        entityID = givenEntityID;
        properties = Entities.getEntityProperties(entityID);
        timeoutID = Script.setTimeout(maybe, minimumDelay);
    };

    this.unload = function () {
         printDebug("unload airship...");
        if (timeoutID !== undefined) {
            Script.clearTimeout(timeoutID);
        }
        if (lightTimeoutID !== undefined) {
            Script.clearTimeout(lightTimeoutID);
        }
        if (spotlight !== null) {
            Entities.deleteEntity(spotlight);
        }
    };

    function makeParticles(position, parent, lifespan) {
        particles = Entities.addEntity({
            type: 'ParticleEffect',
            position: position,  
            parentID: parent,
            color: {
                red: 70,
                green: 70,
                blue: 70
            },
            isEmitting: 1,
            maxParticles: 1000,
            lifetime: lifespan,
            lifespan: lifespan / 3,   
            emitRate: 80,
            emitSpeed: 0,
            speedSpread: 1.0,
            emitRadiusStart: 1,
            polarStart: -Math.PI/8,
            polarFinish: Math.PI/8,
            azimuthStart: -Math.PI/4,
            azimuthFinish: Math.PI/4,
            emitAcceleration: {  x: 0, y: 0, z: 0 },
            particleRadius: 0.25,
            radiusSpread: 0.1,
            radiusStart: 0.3,
            radiusFinish: 0.15,
            colorSpread: {
                red: 100,
                green: 100,
                blue: 0
            },
            colorStart: {
                red: 125,
                green: 125,
                blue: 125
            },
             colorFinish: {
                red: 10,
                green: 10,
                blue: 10
            },
            alpha: 0.5,
            alphaSpread: 0,
            alphaStart: 1,
            alphaFinish: 0,
            emitterShouldTrail: true,
            textures: 'https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png'
        });
    }
})
