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
Script.include("../libraries/utils.js");

var scriptURL = Script.resolvePath('flashlight.js');

var modelURL = "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/flashlight/flashlight.fbx";

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));

var flashlight = Entities.addEntity({
    type: "Model",
    modelURL: modelURL,
    position: center,
    dimensions: {
        x: 0.08,
        y: 0.30,
        z: 0.08
    },
    dynamic: true,
    shapeType: 'box',
    script: scriptURL,
    userData: JSON.stringify({
        grabbableKey: {
            invertSolidWhileHeld: true
        },
        wearable:{joints:{RightHand:[{x:0.0717092975974083,
                                      y:0.1166968047618866,
                                      z:0.07085515558719635},
                                     {x:-0.7195770740509033,
                                      y:0.175227552652359,
                                      z:0.5953742265701294,
                                      w:0.31150275468826294}],
                          LeftHand:[{x:0.0806504637002945,
                                     y:0.09710478782653809,
                                     z:0.08610185235738754},
                                    {x:0.5630447864532471,
                                     y:-0.2545935809612274,
                                     z:0.7855332493782043,
                                     w:0.033170729875564575}]}}
    })
});
