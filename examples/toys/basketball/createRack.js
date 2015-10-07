//
//  createRack.js
//
//  Created by James B. Pollack @imgntn on 10/5/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This is a script that creates a persistent basketball rack.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
Script.include("../../libraries/utils.js");

var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var basketballURL = HIFI_PUBLIC_BUCKET + "models/content/basketball2.fbx";
var collisionSoundURL = HIFI_PUBLIC_BUCKET + "sounds/basketball/basketball.wav";
var rackURL = HIFI_PUBLIC_BUCKET + "models/basketball_hoop/basketball_rack.fbx";
var rackCollisionHullURL = HIFI_PUBLIC_BUCKET + "models/basketball_hoop/rack_collision_hull.obj";
var NUMBER_OF_BALLS = 4;
var DIAMETER = 0.30;
var RESET_DISTANCE = 1;
var MINIMUM_MOVE_LENGTH = 0.05;

var GRABBABLE_DATA_KEY = "grabbableKey";

var rackStartPosition =
    Vec3.sum(MyAvatar.position,
        Vec3.multiplyQbyV(MyAvatar.orientation, {
            x: 0,
            y: 0.0,
            z: -2
        }));

var rack = Entities.addEntity({
    name: 'Basketball Rack',
    type: "Model",
    modelURL: rackURL,
    position: rackStartPosition,
    shapeType: 'compound',
    gravity: {
        x: 0,
        y: -9.8,
        z: 0
    },
    linearDamping: 1,
    dimensions: {
        x: 0.4,
        y: 1.37,
        z: 1.73
    },
    collisionsWillMove: true,
    ignoreForCollisions: false,
    collisionSoundURL: collisionSoundURL,
    compoundShapeURL: rackCollisionHullURL,
    //  scriptURL: rackScriptURL
});


setEntityCustomData(GRABBABLE_DATA_KEY, rack, {
    grabbable: false
});

var nonCollidingBalls = [];
var collidingBalls = [];
var originalBallPositions = [];

function createCollidingBalls() {
    var position = rackStartPosition;

    var i;
    for (i = 0; i < NUMBER_OF_BALLS; i++) {
        var ballPosition = {
            x: position.x,
            y: position.y + DIAMETER * 2,
            z: position.z + (DIAMETER) - (DIAMETER * i)
        };

        var collidingBall = Entities.addEntity({
            type: "Model",
            name: 'Colliding Basketball',
            shapeType: 'Sphere',
            position: ballPosition,
            dimensions: {
                x: DIAMETER,
                y: DIAMETER,
                z: DIAMETER
            },
            restitution: 1.0,
            linearDamping: 0.00001,
            gravity: {
                x: 0,
                y: -9.8,
                z: 0
            },
            collisionsWillMove: true,
            ignoreForCollisions: false,
            modelURL: basketballURL,
        });

        collidingBalls.push(collidingBall);
        originalBallPositions.push(position);
    }
}

function testBallDistanceFromStart() {
    var resetCount = 0;
    collidingBalls.forEach(function(ball, index) {
        var currentPosition = Entities.getEntityProperties(ball, "position").position;
        var originalPosition = originalBallPositions[index];
        var distance = Vec3.subtract(originalPosition, currentPosition);
        var length = Vec3.length(distance);
        if (length > RESET_DISTANCE) {
            Script.setTimeout(function() {
                var newPosition = Entities.getEntityProperties(ball, "position").position;
                var moving = Vec3.length(Vec3.subtract(currentPosition, newPosition));
                if (moving < MINIMUM_MOVE_LENGTH) {
                    resetCount++;
                    if (resetCount === NUMBER_OF_BALLS) {
                        deleteCollidingBalls();
                        createCollidingBalls();
                    }
                }
            }, 200)
        }
    });
}

function deleteEntity(entityID) {
    if (entityID === rack) {
        deleteCollidingBalls();
        Script.clearInterval(distanceCheckInterval);
        Entities.deletingEntity.disconnect(deleteEntity);
    }
}

function deleteCollidingBalls() {
    while (collidingBalls.length > 0) {
        Entities.deleteEntity(collidingBalls.pop());
    }
}

createCollidingBalls();
Entities.deletingEntity.connect(deleteEntity);

var distanceCheckInterval = Script.setInterval(testBallDistanceFromStart, 1000);

function atEnd() {
    Script.clearInterval(distanceCheckInterval);
}

Script.scriptEnding.connect(atEnd);