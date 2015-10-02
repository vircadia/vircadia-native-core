//
//  createKnockoverBlocks.js
//
//  Created by James B. Pollack on 9/29/2015
//  Copyright 2015 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

var blockDimensions={
    x:0.12,
    y:0.25,
    z:0.50
};

var centerPosition = {
    x:
    y:
    z:
};


var sideBlock1_position = {
    x:
    y:
    z:
};



var sideBlock2_position = {
    x:
    y:
    z:
};


var sideBlock1_rotation = Quat.fromPitchYawRollDegrees();
var sideBlock2_rotation = Quat.fromPitchYawRollDegrees();
var topBlock_rotation = Quat.fromPitchYawRollDegrees();

var sideBlock1 = Entities.addEntity();
var sideBlock2 = Entities.addEntity();
var topBlock = Entities.addEntity();
var ground = Entities.addEntity();