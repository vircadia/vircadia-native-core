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

Script.include("../libraries/utils.js");

var WAND_MODEL = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/bubblewand/wand.fbx';
var WAND_COLLISION_SHAPE = 'https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/bubblewand/wand_collision_hull.obj';

var WAND_SCRIPT_URL = Script.resolvePath("wand.js");

//create the wand in front of the avatar 

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));

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
    dynamic: true,
    compoundShapeURL: WAND_COLLISION_SHAPE,
    script: WAND_SCRIPT_URL,
    userData: JSON.stringify({
        grabbableKey: {
            invertSolidWhileHeld: true
        },
        "wearable":{"joints":{"RightHand":[{"x":0.11421211808919907,
                                            "y":0.06508062779903412,
                                            "z":0.06317152827978134},
                                           {"x":-0.7886992692947388,
                                            "y":-0.6108893156051636,
                                            "z":-0.05003821849822998,
                                            "w":0.047579944133758545}],
                              "LeftHand":[{"x":0.03530977666378021,
                                           "y":0.11278322339057922,
                                           "z":0.049768272787332535},
                                          {"x":-0.050609711557626724,
                                           "y":-0.11595471203327179,
                                           "z":0.3554558753967285,
                                           "w":0.9260908961296082}]}}
    })
});

function scriptEnding() {
    Entities.deleteEntity(wand);
}
Script.scriptEnding.connect(scriptEnding);
