//
//  originalPositionResetter.js
//  toybox
//
//  Created by James B. Pollack @imgntn 10/16/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var TARGET_MODEL_URL = HIFI_PUBLIC_BUCKET + "models/ping_pong_gun/target.fbx";
var TARGET_COLLISION_HULL_URL = HIFI_PUBLIC_BUCKET + "models/ping_pong_gun/target_collision_hull.obj";
var TARGET_DIMENSIONS = {
    x: 0.06,
    y: 0.42,
    z: 0.42
};
var TARGET_ROTATION = Quat.fromPitchYawRollDegrees(0, -55.25, 0);

var targetsScriptURL = Script.resolvePath('../ping_pong_gun/wallTarget.js');


var basketballURL = HIFI_PUBLIC_BUCKET + "models/content/basketball2.fbx";

var NUMBER_OF_BALLS = 4;
var BALL_DIAMETER = 0.30;
var RESET_DISTANCE = 1;
var MINIMUM_MOVE_LENGTH = 0.05;

var totalTime = 0;
var lastUpdate = 0;
var UPDATE_INTERVAL = 1 / 5; // 5fps

var Resetter = {
    searchForEntitiesToResetToOriginalPosition: function(searchOrigin, objectName) {
        var ids = Entities.findEntities(searchOrigin, 5);
        var objects = [];
        var i;
        var entityID;
        var name;
        for (i = 0; i < ids.length; i++) {
            entityID = ids[i];
            name = Entities.getEntityProperties(entityID, "name").name;
            if (name === objectName) {
                //we found an object to reset
                objects.push(entityID);
            }
        }
        return objects;
    },
    deleteObjects: function(objects) {
        while (objects.length > 0) {
            Entities.deleteEntity(objects.pop());
        }
    },
    createBasketBalls: function() {
        var position = {
            x: 542.86,
            y: 494.84,
            z: 475.06
        };
        var i;
        var ballPosition;
        var collidingBall;
        for (i = 0; i < NUMBER_OF_BALLS; i++) {
            ballPosition = {
                x: position.x,
                y: position.y + BALL_DIAMETER * 2,
                z: position.z + (BALL_DIAMETER) - (BALL_DIAMETER * i)
            };

            collidingBall = Entities.addEntity({
                type: "Model",
                name: 'Hifi-Basketball',
                shapeType: 'Sphere',
                position: ballPosition,
                dimensions: {
                    x: BALL_DIAMETER,
                    y: BALL_DIAMETER,
                    z: BALL_DIAMETER
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
                userData: JSON.stringify({
                    originalPositionKey: {
                        originalPosition: ballPosition
                    },
                    resetMe: {
                        resetMe: true
                    },
                    grabbable: {
                        invertSolidWhileHeld: true
                    }
                })
            });

        }
    },
    testBallDistanceFromStart: function(balls) {
        var resetCount = 0;
        balls.forEach(function(ball, index) {
            var properties = Entities.getEntityProperties(ball, ["position", "userData"]);
            var currentPosition = properties.position;
            var originalPosition = properties.userData.originalPositionKey.originalPosition;
            var distance = Vec3.subtract(originalPosition, currentPosition);
            var length = Vec3.length(distance);
            if (length > RESET_DISTANCE) {
                Script.setTimeout(function() {
                    var newPosition = Entities.getEntityProperties(ball, "position").position;
                    var moving = Vec3.length(Vec3.subtract(currentPosition, newPosition));
                    if (moving < MINIMUM_MOVE_LENGTH) {
                        if (resetCount === balls.length) {
                            this.deleteObjects(balls);
                            this.createBasketBalls();
                        }
                    }
                }, 200);
            }
        });
    },
    testTargetDistanceFromStart: function(targets) {
        targets.forEach(function(target, index) {
            var properties = Entities.getEntityProperties(target, ["position", "userData"]);
            var currentPosition = properties.position;
            var originalPosition = properties.userData.originalPositionKey.originalPosition;
            var distance = Vec3.subtract(originalPosition, currentPosition);
            var length = Vec3.length(distance);
            if (length > RESET_DISTANCE) {
                Script.setTimeout(function() {
                    var newPosition = Entities.getEntityProperties(target, "position").position;
                    var moving = Vec3.length(Vec3.subtract(currentPosition, newPosition));
                    if (moving < MINIMUM_MOVE_LENGTH) {

                        Entities.deleteEntity(target);

                        var targetProperties = {
                            name: 'Hifi-Target',
                            type: 'Model',
                            modelURL: TARGET_MODEL_URL,
                            shapeType: 'compound',
                            collisionsWillMove: true,
                            dimensions: TARGET_DIMENSIONS,
                            compoundShapeURL: TARGET_COLLISION_HULL_URL,
                            position: originalPosition,
                            rotation: TARGET_ROTATION,
                            script: targetsScriptURL,
                            userData: JSON.stringify({
                                resetMe: {
                                    resetMe: true
                                },
                                grabbableKey: {
                                    grabbable: false
                                },
                                originalPositionKey: originalPosition

                            })
                        };

                        Entities.addEntity(targetProperties);
                    }
                }, 200);
            }

        });

    }
};

function update(deltaTime) {

    if (!Entities.serversExist() || !Entities.canRez()) {
        return;
    }


    totalTime += deltaTime;

    // We don't want to edit the entity EVERY update cycle, because that's just a lot
    // of wasted bandwidth and extra effort on the server for very little visual gain
    if (totalTime - lastUpdate > UPDATE_INTERVAL) {
        //do stuff
        var balls = Resetter.searchForEntitiesToResetToOriginalPosition({
            x: 542.86,
            y: 494.84,
            z: 475.06
        }, "Hifi-Basketball");

        var targets = Resetter.searchForEntitiesToResetToOriginalPosition({
            x: 548.68,
            y: 497.30,
            z: 509.74
        }, "Hifi-Target");

        if (balls.length !== 0) {
            Resetter.testBallDistanceFromStart(balls);
        }

        if (targets.length !== 0) {
            Resetter.testTargetDistanceFromStart(targets);
        }

        lastUpdate = totalTime;
    }

}

Script.update.connect(update);