//  dustSetSpawner.js
//  examples
//
//  Created by Eric Levin on  9/2/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Spawns a set with blocks and a desert-y ground. When blocks (or anything else is thrown), dust particles will kick up at the point the object hits the ground
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */


map = function(value, min1, max1, min2, max2) {
    return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}

orientationOf = function(vector) {
    var Y_AXIS = {
        x: 0,
        y: 1,
        z: 0
    };
    var X_AXIS = {
        x: 1,
        y: 0,
        z: 0
    };

    var theta = 0.0;

    var RAD_TO_DEG = 180.0 / Math.PI;
    var direction, yaw, pitch;
    direction = Vec3.normalize(vector);
    yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * RAD_TO_DEG, Y_AXIS);
    pitch = Quat.angleAxis(Math.asin(-direction.y) * RAD_TO_DEG, X_AXIS);
    return Quat.multiply(yaw, pitch);
}


var ground, wall;
var boxes = [];
var dustSystems = [];
var ZERO_VEC = {x: 0, y: 0, z: 0};

Script.include("../libraries/utils.js");

function spawnGround() {
    var groundModelURL = "https://hifi-public.s3.amazonaws.com/alan/Playa/Ground.fbx";
    var groundPosition = Vec3.sum(MyAvatar.position, {x: 0, y: -2, z: 0});
    ground = Entities.addEntity({
        type: "Model",
        modelURL: groundModelURL,
        shapeType: "box",
        position: groundPosition,
        dimensions: {x: 900, y: 0.82, z: 900},
    });
   // Script.addEventHandler(ground, "collisionWithEntity", entityCollisionWithGround);

}

/*function entityCollisionWithGround(ground, entity, collision) {
    var dVelocityMagnitude = Vec3.length(collision.velocityChange);
    var position = Entities.getEntityProperties(entity, "position").position;
    var particleRadius = map(dVelocityMagnitude, 0.05, 3, 0.5, 2);
    var speed = map(dVelocityMagnitude, 0.05, 3, 0.02, 0.09);
    var displayTime = 400;
    var orientationChange = orientationOf(collision.velocityChange);
    var dustEffect = Entities.addEntity({
        type: "ParticleEffect",
        name: "Dust-Puff",
        position: position,
        color: {red: 195, green: 170, blue: 185},
        lifespan: 3,
        lifetime: 7,//displayTime/1000 * 2, //So we can fade particle system out gracefully
        emitRate: 5, 
        emitSpeed: speed,
        emitAcceleration: ZERO_VEC,
        accelerationSpread: ZERO_VEC,
        isEmitting: true,
        polarStart: Math.PI/2,
        polarFinish: Math.PI/2,
	     emitOrientation: orientationChange,
        radiusSpread: 0.1,
        radiusStart: particleRadius,
        radiusFinish: particleRadius + particleRadius/2,
        particleRadius: particleRadius,
        alpha: 0.45,
        alphaFinish: 0.001,
        textures: "https://hifi-public.s3.amazonaws.com/alan/Playa/Particles/Particle-Sprite-Gen.png"
    });

    dustSystems.push(dustEffect);

    Script.setTimeout(function() {
        var newRadius = 0.05;
        Entities.editEntity(dustEffect, {
            alpha: 0.0
        });
    }, displayTime);
}*/

function spawnBoxes() {
    var boxModelURL = "https://hifi-public.s3.amazonaws.com/alan/Tower-Spawn/Stone-Block.fbx";
    var collisionSoundURL = "https://hifi-public.s3.amazonaws.com/sounds/Collisions-otherorganic/ToyWoodBlock.L.wav";
    var numBoxes = 200;
    var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
    for (var i = 0; i < numBoxes; i++) {
        var position = Vec3.sum(center, {x: Math.random() * numBoxes, y: Math.random() * 3, z: Math.random() * numBoxes })
        var box = Entities.addEntity({
            type: "Model",
            modelURL: boxModelURL,
            collisionSoundURL: collisionSoundURL,
            shapeType: "box",
            position: position,
            dynamic: true,
            dimensions: {x: 1, y: 2, z: 3},
            velocity: {x: 0, y: -.01, z: 0},
            gravity: {x: 0, y: -2.5 - Math.random() * 6, z: 0}
        });

        boxes.push(box);
    }
}

spawnGround();
spawnBoxes();


function cleanup() {
    Entities.deleteEntity(ground);
    boxes.forEach(function(box){
        Entities.deleteEntity(box);
    });
    dustSystems.forEach(function(dustEffect) {
        Entities.deleteEntity(dustEffect);
    })
}

Script.scriptEnding.connect(cleanup);


