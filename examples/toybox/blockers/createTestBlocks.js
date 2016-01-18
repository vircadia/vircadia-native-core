//
//  createTestBlocks.js
//
//  Script Type: Entity Spawner
//  Created by James B. Pollack on 10/5/2015
//  Copyright 2015 High Fidelity, Inc.
//
//
//  This script creates a 'stonehenge' formation of physical blocks for testing knock over properties.
// 
// Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

var BLOCK_GRAVITY = {
    x: 0,
    y: -9.8,
    z: 0
};

var LINEAR_DAMPING = 0.2;

var RED = {
    red: 255,
    green: 0,
    blue: 0
};

var GREEN = {
    red: 0,
    green: 255,
    blue: 0
};

var BLUE = {
    red: 0,
    green: 0,
    blue: 255
};

var blockDimensions = {
    x: 0.12,
    y: 0.25,
    z: 0.50
};

var centerPosition = {
    x: 0,
    y: 0,
    z: 0
};

var DISTANCE_IN_FRONT_OF_ME = 2.0;
var position = Vec3.sum(MyAvatar.position,
    Vec3.multiplyQbyV(MyAvatar.orientation, {
        x: 0,
        y: 0.0,
        z: -DISTANCE_IN_FRONT_OF_ME
    }));

var sideBlock1_position = {
    x: position.x,
    y: position.y,
    z: position.z - 0.25
};

var sideBlock2_position = {
    x: position.x,
    y: position.y,
    z: position.z + 0.25
};

var topBlock_position = {
    x: position.x,
    y: position.y + 0.31,
    z: position.z
};

var sideBlock1_rotation = Quat.fromPitchYawRollDegrees(90, 90, 0);
var sideBlock2_rotation = Quat.fromPitchYawRollDegrees(90, 90, 0);
var topBlock_rotation = Quat.fromPitchYawRollDegrees(0, 0, 90);

var topBlock = Entities.addEntity({
    name: 'Top Block',
    color: BLUE,
    type: 'Box',
    shapeType: 'box',
    dimensions: blockDimensions,
    position: topBlock_position,
    rotation: topBlock_rotation,
    damping: LINEAR_DAMPING,
    gravity: BLOCK_GRAVITY,
    dynamic: true,
    velocity: {
        x: 0,
        y: -0.01,
        z: 0
    }
});

var sideBlock1 = Entities.addEntity({
    name: 'Top Block',
    color: GREEN,
    type: 'Box',
    shapeType: 'box',
    dimensions: blockDimensions,
    position: sideBlock1_position,
    rotation: sideBlock1_rotation,
    damping: LINEAR_DAMPING,
    gravity: BLOCK_GRAVITY,
    dynamic: true
});

var sideBlock2 = Entities.addEntity({
    name: 'Side Block',
    color: GREEN,
    type: 'Box',
    shapeType: 'box',
    dimensions: blockDimensions,
    position: sideBlock2_position,
    rotation: sideBlock2_rotation,
    dynamic: true,
    damping: LINEAR_DAMPING,
    gravity: BLOCK_GRAVITY,
    dynamic: true
});

var ground = Entities.addEntity({
    type: 'Box',
    dimensions: {
        x: 2,
        y: 0.02,
        z: 1
    },
    color: RED,
    position: {
        x: position.x,
        y: position.y - 0.25,
        z: position.z
    }
});
