//
//  whiteBoardSpawner.js
//  examples/painting
//
//  Created by Eric Levina on 10/12/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Run this script to spawn a whiteboard that one can paint on
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */



Script.include("../../libraries/utils.js");
var scriptURL = Script.resolvePath("whiteBoardEntityScript.js?v2");

var rotation = Quat.safeEulerAngles(Camera.getOrientation());
rotation = Quat.fromPitchYawRollDegrees(0, rotation.y, 0);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(rotation)));
center.y += 0.4;
var whiteboard = Entities.addEntity({
    type: "Box",
    position: center,
    rotation: rotation,
    script: scriptURL,
    dimensions: {x: 2, y: 1.5, z: 0.01},
    color: {red: 255, green: 255, blue: 255}
});


function cleanup() {
    Entities.deleteEntity(whiteboard);
}


Script.scriptEnding.connect(cleanup);