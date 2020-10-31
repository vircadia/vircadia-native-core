//
//  Grenade.js
//  examples
//
//  Created by Philip Rosedale on August 20, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var grenadeURL = Script.getExternalPath(Script.ExternalPaths.Assets, "models/props/grenade/grenade.fbx");
var fuseSoundURL = Script.getExternalPath(Script.ExternalPaths.Assets, "sounds/burningFuse.wav");
var boomSoundURL = Script.getExternalPath(Script.ExternalPaths.Assets, "sounds/explosion.wav");

var AudioRotationOffset = Quat.fromPitchYawRollDegrees(0, -90, 0);
var audioOptions = {
  volume: 0.5,
  loop: true
}

var injector = null;

var fuseSound = SoundCache.getSound(fuseSoundURL, audioOptions.isStereo);
var boomSound = SoundCache.getSound(boomSoundURL, audioOptions.isStereo);

var grenade = null;
var particles = null; 
var properties = null;
var originalPosition = null;
var isGrenade = false; 
var isBurning = false; 

var GRAVITY = -9.8;
var TIME_TO_EXPLODE = 2500; 
var DISTANCE_IN_FRONT_OF_ME = 1.0;
var PI = 3.141593;
var DEG_TO_RAD = PI / 180.0;

function makeGrenade() { 
    var position = Vec3.sum(MyAvatar.position,
                        Vec3.multiplyQbyV(MyAvatar.orientation,
                      { x: 0, y: 0.0, z: -DISTANCE_IN_FRONT_OF_ME }));
    var rotation = Quat.multiply(MyAvatar.orientation,
                                  Quat.fromPitchYawRollDegrees(0, -90, 0));
    grenade = Entities.addEntity({
                                type: "Model",
                                position: position,
                                rotation: rotation,
                                dimensions: { x: 0.09,
                                              y: 0.20,
                                              z: 0.09 },
                                dynamic: true,
                                modelURL: grenadeURL,
                                shapeType: "box"
                              });

    properties = Entities.getEntityProperties(grenade);
    audioOptions.position = position;
    audioOptions.orientation = rotation;
    originalPosition = position;
}

function update() {
    if (!grenade) {
        makeGrenade();
    } else {
        var newProperties = Entities.getEntityProperties(grenade);
        if (!isBurning) {
            //  If moved, start fuse 
            var FUSE_START_MOVE = 0.01;
            if (Vec3.length(Vec3.subtract(newProperties.position, originalPosition)) > FUSE_START_MOVE) {
                isBurning = true; 
                //  Create fuse particles 
                particles = Entities.addEntity({
                            type: "ParticleEffect",
                            isEmitting: true,
                            position: newProperties.position,
                            textures: 'https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png',
                            emitRate: 100,
                            polarFinish: 25 * DEG_TO_RAD,
                            color: { red: 200, green: 0, blue: 0 },
                            lifespan: 10.0,
                            visible: true,
                            locked: false
                  
                            });
                //  Start fuse sound 
                injector = Audio.playSound(fuseSound, audioOptions);
                //  Start explosion timer
                Script.setTimeout(boom, TIME_TO_EXPLODE);
                originalPosition = newProperties.position; 
                //  Add gravity 
                Entities.editEntity(grenade, { gravity: {x: 0, y: GRAVITY, z: 0 }});
            }
        }
        
        if (newProperties.type === "Model") {
            if (newProperties.position != properties.position) {
                audioOptions.position = newProperties.position;
            }
            if (newProperties.orientation != properties.orientation) {
                audioOptions.orientation = newProperties.orientation;
            }
            
            properties = newProperties;
            // Update sound location if playing
            if (injector) {
                injector.options = audioOptions;
            }
            if (particles) {
                Entities.editEntity(particles, { position: newProperties.position });
            }
        } else {
            grenade = null;
            Script.update.disconnect(update);
            Script.scriptEnding.connect(scriptEnding);
            scriptEnding();
            Script.stop();
        }
    }
}
function boom() {
    injector.stop();
    isBurning = false; 
    var audioOptions = {
        position: properties.position,
        volume: 0.75,
        loop: false
    }
    Audio.playSound(boomSound, audioOptions);
    Entities.addEntity({
                        type: "ParticleEffect",
                        isEmitting: true,
                        position: properties.position,
                        textures: 'https://raw.githubusercontent.com/ericrius1/SantasLair/santa/assets/smokeparticle.png',
                        emitRate: 200,
                        polarFinish: 25 * DEG_TO_RAD,
                        color: { red: 255, green: 255, blue: 0 },
                        lifespan: 2.0,
                        visible: true,
                        lifetime: 2,
                        locked: false
              
                        });
    var BLAST_RADIUS = 20.0;
    var LIFT_DEPTH = 2.0;
    var epicenter = properties.position;
    epicenter.y -= LIFT_DEPTH;
    blowShitUp(epicenter, BLAST_RADIUS);
    deleteStuff(); 
}

function blowShitUp(position, radius) {
    var stuff = Entities.findEntities(position, radius);
    var numMoveable = 0;
    var STRENGTH = 3.5;
    var SPIN_RATE = 20.0;
    for (var i = 0; i < stuff.length; i++) {
        var properties = Entities.getEntityProperties(stuff[i]);
        if (properties.dynamic) {
            var diff = Vec3.subtract(properties.position, position);
            var distance = Vec3.length(diff);
            var velocity = Vec3.sum(properties.velocity, Vec3.multiply(STRENGTH * 1.0 / distance, Vec3.normalize(diff)));
            var angularVelocity = { x: Math.random() * SPIN_RATE, y: Math.random() * SPIN_RATE, z: Math.random() * SPIN_RATE };
            angularVelocity = Vec3.multiply( 1.0 / distance, angularVelocity);
            Entities.editEntity(stuff[i], { velocity: velocity, 
                                            angularVelocity: angularVelocity });
        }
    }
}

function scriptEnding() {
    deleteStuff();
}

function deleteStuff() { 
    if (grenade != null) {
        Entities.deleteEntity(grenade);
        grenade = null;
    }
    if (particles != null) {
        Entities.deleteEntity(particles);
        particles = null;
    }
}

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);

