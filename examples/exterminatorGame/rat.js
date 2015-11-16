//
//  Rat.js
//  examples/toybox/entityScripts
//
//  Created by Eric Levin on11/11/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global Rat, print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */


(function() {

    var _this;
    Rat = function() {
        _this = this;
        this.forceMultiplier = 1;
    };

    Rat.prototype = {

        onShot: function(myId, params) {
            var data = JSON.parse(params[0]);
            var force = Vec3.multiply(data.forceDirection, this.forceMultiplier);
            Entities.editEntity(this.entityID, {
                velocity: force
            });

            Script.setTimeout(function() {
                var vel = Entities.getEntityProperties(_this.entityID, 'velocity').velocity;
            }, 1000);
        },

        preload: function(entityID) {
            this.entityID = entityID;
        },

    };

    // entity scripts always need to return a newly constructed object of our type
    return new Rat();
});