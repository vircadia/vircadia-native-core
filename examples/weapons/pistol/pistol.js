//
//  pistol.js
//  examples/toybox/entityScripts
//
//  Created by Eric Levin on11/11/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */


(function() {
    var scriptURL = Script.resolvePath('pistol.js');
    var _this;
    Pistol = function() {
        _this = this;
        this.equipped = false;
        this.forceMultiplier = 1;
    };

    Pistol.prototype = {

        equip: function() {
            this.equipped = true;
            print("EQUIP")

        },
        unequip: function() {
            this.equipped = true;
        },

        preload: function(entityID) {
            this.entityID = entityID;
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    print("TOOOss")
    return new Pistol();
});