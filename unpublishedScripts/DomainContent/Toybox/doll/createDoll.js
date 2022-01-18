//  createDoll.js
//  
//  Script Type: Entity
//  Created by James B. Pollack @imgntn 9/23/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Creates a doll entity in front of you.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

function createDoll() {
    var modelURL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/doll/bboy2.fbx";

    var scriptURL = Script.resolvePath("doll.js");

    var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
        x: 0,
        y: 0.5,
        z: 0
    }), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));

    var naturalDimensions = {
        x: 1.63,
        y: 1.67,
        z: 0.26
    };

    var desiredDimensions = Vec3.multiply(naturalDimensions, 0.15);

    var doll = Entities.addEntity({
        type: "Model",
        name: "doll",
        modelURL: modelURL,
        script: scriptURL,
        position: center,
        shapeType: 'box',
        dimensions: desiredDimensions,
        gravity: {
            x: 0,
            y: 0,
            z: 0
        },
        velocity: {
            x: 0,
            y: 0,
            z: 0
        },
        dynamic: true,
        userData: JSON.stringify({
            grabbableKey: {
                invertSolidWhileHeld: true
            }
        })
    });
    return doll;
}

createDoll();
