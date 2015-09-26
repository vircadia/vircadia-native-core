//
//  createFlashlight.js
//  examples/entityScripts
//
//  Created by Sam Gateau on 9/9/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  This is a toy script that create a flashlight entity that lit when grabbed
//  This can be run from an interface and the flashlight will get deleted from the domain when quitting
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
Script.include("../../utilities.js");
Script.include("../../libraries/utils.js");

var groundURL = "https://hifi-public.s3.amazonaws.com/eric/models/woodFloor.fbx";
var basketballURL = "https://hifi-public.s3.amazonaws.com/models/content/basketball2.fbx";
var hoopURL = "http://hifi-public.s3.amazonaws.com/models/basketball/new_hoop_2.fbx";
var hoopCollisionHullURL = "http://hifi-public.s3.amazonaws.com/models/basketball/new_basketball_hoop_collision_hull.obj";
var ballCollisionSound = "https://hifi-public.s3.amazonaws.com/sounds/basketball/basketball.wav";

var basePosition = {
    x: 0,
    y: 0,
    z: 0
};

var hoopStartPosition = {
    x: 0,
    y: 3.25,
    z: 0
};


var ground = Entities.addEntity({
    type: "Model",
    modelURL: groundURL,
    dimensions: {
        x: 100,
        y: 2,
        z: 100
    },
    position: basePosition,
    shapeType: 'box'
});

var BALL_DIAMETER = 0.30;
var DISTANCE_IN_FRONT_OF_ME = 1.0;

var ballPosition = Vec3.sum(MyAvatar.position,
    Vec3.multiplyQbyV(MyAvatar.orientation, {
        x: 0,
        y: 0.0,
        z: -DISTANCE_IN_FRONT_OF_ME
    }));

var ballRotation = Quat.multiply(MyAvatar.orientation,
    Quat.fromPitchYawRollDegrees(0, -90, 0));

var basketball = Entities.addEntity({
    type: "Model",
    position: ballPosition,
    rotation: ballRotation,
    dimensions: {
        x: BALL_DIAMETER,
        y: BALL_DIAMETER,
        z: BALL_DIAMETER
    },
    gravity: {
        x: 0,
        y: -9.8,
        z: 0
    },
    collisionsWillMove: true,
    collisionSoundURL: ballCollisionSound,
    modelURL: basketballURL,
    restitution: 1.0,
    linearDamping: 0.00001,
    shapeType: "sphere"
});


var hoop = Entities.addEntity({
    type: "Model",
    modelURL: hoopURL,
    position: hoopStartPosition,
    shapeType: 'compound',
    gravity: {
        x: 0,
        y: -9.8,
        z: 0
    },
    dimensions: {
        x: 1.89,
        y: 3.99,
        z: 3.79
    },
    compoundShapeURL: hoopCollisionHullURL
});