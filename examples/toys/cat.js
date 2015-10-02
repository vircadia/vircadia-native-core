//
//  cat.js
//  examples/toybox/entityScripts
//
//  Created by Eric Levin on 9/21/15.
//  Copyright 2015 High Fidelity, Inc.
//

//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global Cat, print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */


(function() {

var _this;
    Cat = function() {
        _this=this;
        this.meowSound = SoundCache.getSound("https://s3.amazonaws.com/hifi-public/sounds/Animals/cat_meow.wav");
    };

    Cat.prototype = {
        isMeowing: false,
        injector: null,
        startTouch: function() {
            if (this.isMeowing !== true) {
                this.meow();
                this.isMeowing = true;
            }

        },

        update: function() {
            if (_this.injector !== null) {
                _this.isMeowing = this.injector.isPlaying;
                    if (_this.isMeowing === false) {
                        _this.injector = null;
                    }
            }
        
        },

        meow: function() {
            this.injector = Audio.playSound(this.meowSound, {
                position: this.position,
                volume: 0.1
            });
        },

        preload: function(entityID) {
            this.entityID = entityID;
            this.position = Entities.getEntityProperties(this.entityID, "position").position;
            Script.update.connect(this.update);
        },

        unload: function() {
            Script.update.disconnect(this.update);
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new Cat();
});
