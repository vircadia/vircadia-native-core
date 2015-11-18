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

        startEquip: function(id, params) {
            print("HAND ", params[0]);
            this.equipped = true;
            this.hand = JSON.parse(params[0]);

        },
        unequip: function() {
            print("UNEQUIP")
            this.equipped = false;
        },

        preload: function(entityID) {
            this.entityID = entityID;
            print("INIT CONTROLLER MAPIING")
            this.initControllerMapping();
        },

        triggerPress: function(hand, value) {
            print("TRIGGER PRESS");
            if (this.hand === hand && value === 1) {
                //We are pulling trigger on the hand we have the gun in, so fire
                this.fire();
            }
        },

        fire: function() {
            print("FIRE!");
        },

        initControllerMapping: function() {
            this.mapping = Controller.newMapping();
            this.mapping.from(Controller.Standard.LT).hysteresis(0.0, 0.5).to(function(value) {
                _this.triggerPress(0, value);
            });


            this.mapping.from(Controller.Standard.RT).hysteresis(0.0, 0.5).to(function(value) {
                _this.triggerPress(1, value);
            });
            this.mapping.enable();

        },

        unload: function() {
            this.mapping.disable();
        }

    };

    // entity scripts always need to return a newly constructed object of our type
    print("TOOOss")
    return new Pistol();
});