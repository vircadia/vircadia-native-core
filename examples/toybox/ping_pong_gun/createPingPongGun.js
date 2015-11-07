//  createPingPongGun.js
//
//  Script Type: Entity Spawner
//  Created by James B. Pollack on  9/30/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a gun that shoots ping pong balls when you pull the trigger on a hand controller.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */
Script.include("../../utilities.js");

var scriptURL = Script.resolvePath('pingPongGun.js');

var MODEL_URL = 'http://hifi-public.s3.amazonaws.com/models/ping_pong_gun/ping_pong_gun.fbx'
var COLLISION_HULL_URL = 'http://hifi-public.s3.amazonaws.com/models/ping_pong_gun/ping_pong_gun_collision_hull.obj';

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));

var pingPongGun = Entities.addEntity({
    type: "Model",
    modelURL: MODEL_URL,
    shapeType:'box',
    // shapeType: 'compound',
    // compoundShapeURL: COLLISION_HULL_URL,
    script: scriptURL,
    position: center,
    dimensions: {
        x: 0.08,
        y: 0.21,
        z: 0.47
    },
    collisionsWillMove: true,
});

function cleanUp() {
    Entities.deleteEntity(pingPongGun);
}
Script.scriptEnding.connect(cleanUp);