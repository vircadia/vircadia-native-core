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

Script.include("../libraries/utils.js");

var basketballURL ="http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/basketball/basketball2.fbx";
var collisionSoundURL = "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/basketball/basketball.wav";
var rackURL = "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/basketball/basketball_rack.fbx";
var rackCollisionHullURL ="http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/basketball/rack_collision_hull.obj";
var NUMBER_OF_BALLS = 4;
var DIAMETER = 0.30;
var RESET_DISTANCE = 1;
var MINIMUM_MOVE_LENGTH = 0.05;

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
    damping: 1,
    dimensions: {
        x: 0.4,
        y: 1.37,
        z: 1.73
    },
    dynamic: true,
    collisionless: false,
    collisionSoundURL: collisionSoundURL,
    compoundShapeURL: rackCollisionHullURL,
    userData: JSON.stringify({
        grabbableKey: {
            grabbable: false
        }
    })
});

var balls = [];
var originalBallPositions = [];

function createBalls() {
    var position = rackStartPosition;

    var i;
    for (i = 0; i < NUMBER_OF_BALLS; i++) {
        var ballPosition = {
            x: position.x,
            y: position.y + DIAMETER * 2,
            z: position.z + (DIAMETER) - (DIAMETER * i)
        };

        var ball = Entities.addEntity({
            type: "Model",
            name: 'Hifi-Basketball',
            shapeType: 'Sphere',
            position: ballPosition,
            dimensions: {
                x: DIAMETER,
                y: DIAMETER,
                z: DIAMETER
            },
            restitution: 1.0,
            damping: 0.00001,
            gravity: {
                x: 0,
                y: -9.8,
                z: 0
            },
            dynamic: true,
            collisionless: false,
            modelURL: basketballURL,
            userData: JSON.stringify({
                grabbableKey: {
                    invertSolidWhileHeld: true
                }
            })
        });

        balls.push(ball);
        originalBallPositions.push(position);
    }
}

function testBallDistanceFromStart() {
    var resetCount = 0;
    balls.forEach(function(ball, index) {
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
                        deleteBalls();
                        createBalls();
                    }
                }
            }, 200)
        }
    });
}

function deleteEntity(entityID) {
    if (entityID === rack) {
        deleteBalls();
        Script.clearInterval(distanceCheckInterval);
        Entities.deletingEntity.disconnect(deleteEntity);
    }
}

function deleteBalls() {
    while (balls.length > 0) {
        Entities.deleteEntity(balls.pop());
    }
}

createBalls();
Entities.deletingEntity.connect(deleteEntity);

var distanceCheckInterval = Script.setInterval(testBallDistanceFromStart, 1000);

function atEnd() {
    Script.clearInterval(distanceCheckInterval);
}

Script.scriptEnding.connect(atEnd);
