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
Script.include("../../utilities.js");
Script.include("../../libraries/utils.js");
var scriptURL = Script.resolvePath('wallTarget.js');

var MODEL_URL = 'http://hifi-public.s3.amazonaws.com/models/ping_pong_gun/target.fbx';
var COLLISION_HULL_URL = 'http://hifi-public.s3.amazonaws.com/models/ping_pong_gun/target_collision_hull.obj';

var RESET_DISTANCE = 1;
var TARGET_USER_DATA_KEY = 'hifi-ping_pong_target';
var NUMBER_OF_TARGETS = 6;
var TARGETS_PER_ROW = 3;

var TARGET_DIMENSIONS = {
    x: 0.03,
    y: 0.21,
    z: 0.21
};

var VERTICAL_SPACING = 0.3;
var NUM_ROWS = 2;
var NUM_COLUMNS = 3;
var spacingVector = {x: 1.4, y: 0, z: -0.93};
var HORIZONTAL_SPACING = Vec3.multiply(Vec3.normalize(spacingVector), 0.5);
var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));


var startPosition = {
    x: 548.68,
    y: 497.30,
    z: 509.74
}

// var rotation = Quat.fromPitchYawRollDegrees(0, -54, 0.0);

// var startPosition = center;

var targetIntervalClearer = Entities.addEntity({
    name: 'Target Interval Clearer - delete me to clear',
    type: 'Box',
    position: startPosition,
    dimensions: {
        x: 1,
        y: 1,
        z: 1
    },
    visible: false,
    ignoreForCollisions: true,
})
var targets = [];

var originalPositions = [];

function addTargets() {
    var i, rowIndex, columnIndex;
    var row = -1;
    var rotation = Quat.fromPitchYawRollDegrees(-80, -48, -11);
    for (rowIndex = 0; rowIndex < NUM_ROWS; rowIndex++) {
        for (columnIndex = 0; columnIndex < NUM_COLUMNS; columnIndex++) {

            var position = Vec3.sum(startPosition, Vec3.multiply(HORIZONTAL_SPACING, columnIndex));

            originalPositions.push(position);
            var targetProperties = {
                name: 'Target',
                type: 'Model',
                modelURL: MODEL_URL,
                shapeType: 'compound',
                collisionsWillMove: true,
                dimensions: TARGET_DIMENSIONS,
                compoundShapeURL: COLLISION_HULL_URL,
                position: position,
                rotation:rotation,
                script: scriptURL
            };
            targets.push(Entities.addEntity(targetProperties));
        }
        startPosition = Vec3.sum(startPosition, {x: 0, y: VERTICAL_SPACING, z: 0});
    }
}


function testTargetDistanceFromStart() {
    print('TEST TARGET DISTANCE FROM START')
    var resetCount = 0;
    targets.forEach(function(target, index) {
        var currentPosition = Entities.getEntityProperties(target, "position").position;
        var originalPosition = originalPositions[index];
        var distance = Vec3.subtract(originalPosition, currentPosition);
        var length = Vec3.length(distance);
        if (length > RESET_DISTANCE) {
            print('SHOULD RESET THIS! at   ' + originalPositions[index])
            Entities.deleteEntity(target);
            var targetProperties = {
                name: 'Target',
                type: 'Model',
                modelURL: MODEL_URL,
                shapeType: 'compound',
                collisionsWillMove: true,
                dimensions: TARGET_DIMENSIONS,
                compoundShapeURL: COLLISION_HULL_URL,
                position: originalPositions[index],
                // rotation:rotation,
                script: scriptURL
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
var distanceCheckInterval = Script.setInterval(testTargetDistanceFromStart, 1000);

addTargets();

function atEnd() {
    Script.clearInterval(distanceCheckInterval);
    deleteTargets();
}

Script.scriptEnding.connect(atEnd);