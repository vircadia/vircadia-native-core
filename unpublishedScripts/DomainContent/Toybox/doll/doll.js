//  doll.js
//  
//  Script Type: Entity
//  Created by Eric Levin on 9/21/15.
//  Additions by James B. Pollack @imgntn on 9/24/15
//  Copyright 2015 High Fidelity, Inc.
//
//  This entity script plays an animation and a sound while you hold it.
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Known issues:  If you pass the doll between hands, animation can get into a weird state.  We want to prevent the animation from starting again when you switch hands, but when you switch mid-animation your hand at release is still the first hand and not the current hand.
/*global MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt */

(function() {
    Script.include("../libraries/utils.js");
    var _this;
    // this is the "constructor" for the entity as a JS object we don't do much here
    var Doll = function() {
        _this = this;
        this.screamSounds = [SoundCache.getSound("http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/doll/KenDoll_1%2303.wav")];
    };

    Doll.prototype = {
        audioInjector: null,
        isGrabbed: false,

        startNearGrab: function(entityID, args) {
            this.hand = args[0];
            avatarID = args[1];

            Entities.editEntity(this.entityID, {
                animation: {
                    url: "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/doll/zombie_scream.fbx",
                    running: true
                }
            });

            var position = Entities.getEntityProperties(this.entityID, "position").position;
            this.audioInjector = Audio.playSound(this.screamSounds[randInt(0, this.screamSounds.length)], {
                position: position,
                volume: 0.1
            });

            this.isGrabbed = true;
            this.initialHand = this.hand;
        },
        startEquip: function(id, params) {
            this.startNearGrab(id, params);
        },

        continueNearGrab: function(entityID, args) {
            var props = Entities.getEntityProperties(this.entityID, ["position"]);
            var audioOptions = {
                position: props.position
            };
            this.audioInjector.options = audioOptions;
        },
        continueEquip: function(entityID, args) {
            this.continueNearGrab(entityID, args);
        },

        releaseGrab: function(entityID, args) {
            if (this.isGrabbed === true && this.hand === this.initialHand) {
                this.audioInjector.stop();
                Entities.editEntity(this.entityID, {
                    animation: {
                        // Providing actual model fbx for animation used to work, now contorts doll into a weird ball
                        // See bug:  https://app.asana.com/0/26225263936266/70097355490098
                        // url: "http://hifi-public.s3.amazonaws.com/models/Bboys/bboy2/bboy2.fbx",
                        running: false,
                    }
                });

                this.isGrabbed = false;
            }
        },
        releaseEquip: function(entityID, args) {
            this.releaseGrab(entityID, args);
        },

        preload: function(entityID) {
            this.entityID = entityID;
        },
    };
    // entity scripts always need to return a newly constructed object of our type
    return new Doll();
});
