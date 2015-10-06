//
//  createRack.js
//
//  Created by James B. Pollack on10/5/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This is a script that creates a persistent basketball rack.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var basketballURL = HIFI_PUBLIC_BUCKET + "models/content/basketball2.fbx";
var collisionSoundURL = HIFI_PUBLIC_BUCKET + "sounds/basketball/basketball.wav";
var rackURL = HIFI_PUBLIC_BUCKET + "models/basketball_hoop/basketball_rack.fbx";
var rackCollisionHullURL = HIFI_PUBLIC_BUCKET + "models/basketball_hoop/basketball_hoop_collision_hull.obj";
var DIAMETER = 0.30;

var rackStartPosition =
    Vec3.sum(MyAvatar.position,
        Vec3.multiplyQbyV(MyAvatar.orientation, {
            x: 0,
            y: 0.0,
            z: -2
        }));

var rack = Entities.addEntity({
    type: "Model",
    modelURL: rackURL,
    position: rackStartPosition,
    shapeType: 'compound',
    // gravity: {
    //     x: 0,
    //     y: -9.8,
    //     z: 0
    // },
    dimensions: {
        x: 0.4,
        y: 1.37,
        z: 1.73
    },
    collisionsWillMove: false,
    ignoreForCollisions: true,
    compoundShapeURL: rackCollisionHullURL
});

var nonCollidingBalls = [];
var collidingBalls = [];

function createNonCollidingBalls() {
    var i;
    var j;
    var position = rackStartPosition;
    for (i = 0; i < 4; i++) {
        var nonCollidingBall = Entities.addEntity({
            type: "Model",
            name: 'Static Basketball',
            position: {
                x: position.x,
                y: position.y,
                z: position.z + (DIAMETER) - (DIAMETER * i)
            },
            dimensions: {
                x: DIAMETER,
                y: DIAMETER,
                z: DIAMETER
            },
            collisionsWillMove: true,
            ignoreForCollisions: false,
            modelURL: basketballURL
        });
        nonCollidingBalls.push(nonCollidingBall);
    }
    for (i = 0; i < 4; i++) {
        var nonCollidingBall = Entities.addEntity({
            type: "Model",
            name: 'Static Basketball',
            position: {
                x: position.x,
                y: position.y + DIAMETER,
                z: position.z + (DIAMETER) - (DIAMETER * i)
            },
            dimensions: {
                x: DIAMETER,
                y: DIAMETER,
                z: DIAMETER
            },
            collisionsWillMove: true,
            ignoreForCollisions: false,
            modelURL: basketballURL
        });
        nonCollidingBalls.push(nonCollidingBall);
    }
}

function createCollidingBalls() {
    var position = rackStartPosition;
    for (i = 0; i < 4; i++) {
        var collidingBall = Entities.addEntity({
            type: "Model",
            name: 'Colliding Basketball',
            position: {
                x: position.x,
                y: position.y + DIAMETER * 2,
                z: position.z + (DIAMETER) - (DIAMETER * i)
            },
            dimensions: {
                x: DIAMETER,
                y: DIAMETER,
                z: DIAMETER
            },
            // restitution: 1.0,
            // linearDamping: 0.00001,
            gravity: {
                x: 0,
                y: -9.8,
                z: 0
            },
            collisionsWillMove: true,
            ignoreForCollisions: false,
            modelURL: basketballURL
        });
        collidingBalls.push(collidingBall);
    }
}

function adjustBallPositions() {
    var i;
    for (i = 0; i < nonCollidingBalls.length; i++) {
        Entities.editEntity(nonCollidingBalls[i])
    }
}

createNonCollidingBalls();
createCollidingBalls();