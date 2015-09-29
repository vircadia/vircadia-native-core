//
//  createHoop.js
//  examples/entityScripts
//
//  Created by James B. Pollack on 9/29/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This is a script that creates a persistent basketball hoop with a working collision hull.  Feel free to move it.
//  Run basketball.js to make a basketball.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

var hoopURL = "http://hifi-public.s3.amazonaws.com/models/basketball_hoop/basketball_hoop.fbx";
var hoopCollisionHullURL = "http://hifi-public.s3.amazonaws.com/models/basketball_hoop/basketball_hoop_collision_hull.obj";

var hoopStartPosition =
    Vec3.sum(MyAvatar.position,
        Vec3.multiplyQbyV(MyAvatar.orientation, {
            x: 0,
            y: 0.0,
            z: -2
        }));

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

