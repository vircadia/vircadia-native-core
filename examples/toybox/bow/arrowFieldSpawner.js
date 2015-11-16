//  arrowFieldSpawner.js
//  examples
//
//  Created by James B. Pollack @imgntn on 11/16/2015
//  Copyright 2015 High Fidelity, Inc.
//
// Spawns ground, targets, and fire sources.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var ground, wall;
var boxes = [];
var dustSystems = [];
var ZERO_VEC = {
    x: 0,
    y: 0,
    z: 0
};

Script.include("../libraries/utils.js");

var startPosition = {
    x: 0,
    y: 0,
    z: 0
}

function spawnGround() {
    var groundModelURL = "https://hifi-public.s3.amazonaws.com/alan/Playa/Ground.fbx";
    var groundPosition = Vec3.sum(startPosition, {
        x: 0,
        y: -2,
        z: 0
    });
    ground = Entities.addEntity({
        type: "Model",
        modelURL: groundModelURL,
        shapeType: "box",
        position: groundPosition,
        dimensions: {
            x: 900,
            y: 0.82,
            z: 900
        },
    });

}

function spawnBoxes() {
    var boxModelURL = "http://hifi-public.s3.amazonaws.com/models/ping_pong_gun/target.fbx";
    var COLLISION_HULL_URL = 'http://hifi-public.s3.amazonaws.com/models/ping_pong_gun/target_collision_hull.obj';
    var TARGET_DIMENSIONS = {
        x: 0.12,
        y: 0.84,
        z: 0.84
    };

    var collisionSoundURL = "https://hifi-public.s3.amazonaws.com/sounds/Collisions-otherorganic/ToyWoodBlock.L.wav";
    var numBoxes = 200;
    for (var i = 0; i < numBoxes; i++) {
        var position = Vec3.sum(startPosition, {
            x: Math.random() * numBoxes,
            y: Math.random() * 2,
            z: Math.random() * numBoxes
        })
        var box = Entities.addEntity({
            type: "Model",
            modelURL: boxModelURL,
            collisionSoundURL: collisionSoundURL,
            shapeType: "compound",
            compoundShapeURL: COLLISION_HULL_URL,
            position: position,
            collisionsWillMove: true,
            dimensions: TARGET_DIMENSIONS,

        });

        Script.addEventHandler(box, "collisionWithEntity", boxCollision);
        boxes.push(box);
    }
}

function boxCollision(me, other, collision) {
    Entities.editEntity(me, {
        gravity: {
            x: 0,
            y: -9.8,
            z: 0
        }
    })
}

spawnGround();
spawnBoxes();


function cleanup() {
    Entities.deleteEntity(ground);
    boxes.forEach(function(box) {
        Entities.deleteEntity(box);
    });

}

Script.scriptEnding.connect(cleanup);