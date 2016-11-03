//  createTargets.js
// 
//  Script Type: Entity Spawner
//  Created by James B. Pollack on  9/30/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates targets that fall down when you shoot them and then automatically reset to their original position.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

Script.include("../libraries/utils.js");
var scriptURL = Script.resolvePath('wallTarget.js');

var MODEL_URL = 'http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/ping_pong_gun/target.fbx';
var COLLISION_HULL_URL = 'http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/ping_pong_gun/target_collision_hull.obj';
var MINIMUM_MOVE_LENGTH = 0.05;
var RESET_DISTANCE = 0.5;

var TARGET_USER_DATA_KEY = 'hifi-ping_pong_target';
var NUMBER_OF_TARGETS = 6;
var TARGETS_PER_ROW = 3;

var TARGET_DIMENSIONS = {
    x: 0.06,
    y: 0.42,
    z: 0.42
};

var VERTICAL_SPACING = TARGET_DIMENSIONS.y + 0.5;
var HORIZONTAL_SPACING = TARGET_DIMENSIONS.z + 0.5;


var startPosition = {
    x: 548.68,
    y: 497.30,
    z: 509.74
};

startPosition = MyAvatar.position;

var rotation = Quat.fromPitchYawRollDegrees(0, -55.25, 0);

var targetIntervalClearer = Entities.addEntity({
    name: 'Target Interval Clearer - delete me to clear',
    type: 'Box',
    position: startPosition,
    dimensions: TARGET_DIMENSIONS,
    color: {
        red: 0,
        green: 255,
        blue: 0
    },
    rotation: rotation,
    visible: false,
    dynamic: false,
    collisionless: true,
});

var targets = [];

var originalPositions = [];

var lastPositions = [];

function addTargets() {
    var i;
    var row = -1;

    for (i = 0; i < NUMBER_OF_TARGETS; i++) {

        if (i % TARGETS_PER_ROW === 0) {
            row++;
        }

        var vHat = Quat.getFront(rotation);
        var spacer = HORIZONTAL_SPACING * (i % TARGETS_PER_ROW) + (row * HORIZONTAL_SPACING / 2);
        var multiplier = Vec3.multiply(spacer, vHat);
        var position = Vec3.sum(startPosition, multiplier);
        position.y = startPosition.y - (row * VERTICAL_SPACING);

        originalPositions.push(position);
        lastPositions.push(position);

        var targetProperties = {
            name: 'Target',
            type: 'Model',
            modelURL: MODEL_URL,
            shapeType: 'compound',
            dynamic: true,
            dimensions: TARGET_DIMENSIONS,
            compoundShapeURL: COLLISION_HULL_URL,
            position: position,
            rotation: rotation,
            script: scriptURL
        };

        targets.push(Entities.addEntity(targetProperties));
    }
}

function testTargetDistanceFromStart() {
    targets.forEach(function(target, index) {

        var currentPosition = Entities.getEntityProperties(target, "position").position;
        var originalPosition = originalPositions[index];
        var distance = Vec3.subtract(originalPosition, currentPosition);
        var length = Vec3.length(distance);

        var moving = Vec3.length(Vec3.subtract(currentPosition, lastPositions[index]));

        lastPositions[index] = currentPosition;

        if (length > RESET_DISTANCE && moving < MINIMUM_MOVE_LENGTH) {

            Entities.deleteEntity(target);

            var targetProperties = {
                name: 'Target',
                type: 'Model',
                modelURL: MODEL_URL,
                shapeType: 'compound',
                dynamic: true,
                dimensions: TARGET_DIMENSIONS,
                compoundShapeURL: COLLISION_HULL_URL,
                position: originalPositions[index],
                rotation: rotation,
                script: scriptURL,
                userData: JSON.stringify({
                    grabbableKey: {
                        grabbable: false
                    }
                })
            };

            targets[index] = Entities.addEntity(targetProperties);

        }
    });
}


function deleteEntity(entityID) {
    if (entityID === targetIntervalClearer) {
        deleteTargets();
        Script.clearInterval(distanceCheckInterval);
        Entities.deletingEntity.disconnect(deleteEntity);
    }
}

function deleteTargets() {
    while (targets.length > 0) {
        Entities.deleteEntity(targets.pop());
    }
    Entities.deleteEntity(targetIntervalClearer);
}

Entities.deletingEntity.connect(deleteEntity);
var distanceCheckInterval = Script.setInterval(testTargetDistanceFromStart, 500);

addTargets();

function atEnd() {
    Script.clearInterval(distanceCheckInterval);
    deleteTargets();
}

Script.scriptEnding.connect(atEnd);
