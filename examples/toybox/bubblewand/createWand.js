//  createWand.js
//  part of bubblewand
//
//  Script Type: Entity Spawner
//  Created by James B. Pollack @imgntn -- 09/03/2015
//  Copyright 2015 High Fidelity, Inc.
// 
//  Loads a wand model and attaches the bubble wand behavior.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

Script.include("../../utilities.js");
Script.include("../../libraries/utils.js");

var WAND_MODEL = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/wand.fbx';
var WAND_COLLISION_SHAPE = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/collisionHull.obj';
var WAND_SCRIPT_URL = Script.resolvePath("wand.js");

//create the wand in front of the avatar 

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {x: 0, y: 0.5, z: 0}), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));

var wand = Entities.addEntity({
    name: 'Bubble Wand',
    type: "Model",
    modelURL: WAND_MODEL,
    position: center,
    gravity: {
        x: 0,
        y: 0,
        z: 0,
    },
    dimensions: {
        x: 0.05,
        y: 0.25,
        z: 0.05
    },
    //must be enabled to be grabbable in the physics engine
    collisionsWillMove: true,
    compoundShapeURL: WAND_COLLISION_SHAPE,
    script: WAND_SCRIPT_URL
});